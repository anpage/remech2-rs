//! Huffman tree decoding: small byte-trees, the four header trees with
//! their big flattened trees, the table walk, and the move-to-front
//! recency cache.
//!
//! Two structural properties matter for real-world files and are enforced
//! here:
//!
//! - A tree whose presence flag is 0 consumes **only** that bit — there is
//!   no terminator bit to skip for an absent tree.
//! - A big tree's declared size bounds its backing array; it is **not** an
//!   exact entry count. Real trees routinely under-fill it, which is fine.
//!   Only overflow is an error.

use alloc::vec::Vec;

use crate::Error;
use crate::bitreader::BitReader;

/// Flag stored in internal nodes of a flattened big tree: the entry is a
/// node, and the low 31 bits hold the offset to the right subtree relative
/// to the node itself.
const NODE_FLAG: u32 = 0x8000_0000;

/// Maximum depth of a small tree (and therefore the longest code).
const MAX_SMALL_DEPTH: u32 = 27;
/// Maximum leaves in a small tree.
const MAX_SMALL_LEAVES: usize = 256;
/// Maximum recursion depth of a big tree.
const MAX_BIG_DEPTH: u32 = 500;

/// A small byte-tree, stored flattened with the same layout as the big
/// trees: internal nodes hold [`NODE_FLAG`] | (entry count of their left
/// subtree), leaves hold their value. Decoding walks the tree bit by bit —
/// the codes are the actual tree paths, which a canonical length-only
/// assignment does **not** reproduce in general.
///
/// A tree with a single leaf needs no walk at all: its root is a leaf, so
/// decoding yields the value without consuming any bits. An absent tree is
/// the constant 0.
#[derive(Debug, Clone)]
pub(crate) enum ByteTree {
    /// Absent tree: yields 0 without consuming any bits.
    Constant(u8),
    /// A flattened pre-order tree.
    Tree(Vec<u32>),
}

impl ByteTree {
    /// An absent byte-tree: constant 0.
    pub(crate) const fn absent() -> Self {
        Self::Constant(0)
    }

    pub(crate) fn decode(&self, br: &mut BitReader<'_>) -> Result<u8, Error> {
        match self {
            Self::Constant(v) => Ok(*v),
            Self::Tree(entries) => {
                let mut index = 0usize;
                loop {
                    let entry = *entries
                        .get(index)
                        .ok_or(Error::InvalidHuffmanTree("table walk out of bounds"))?;
                    if entry & NODE_FLAG == 0 {
                        return Ok(u8::try_from(entry).unwrap_or(0));
                    }
                    if br.read_bit()? != 0 {
                        index = index
                            .checked_add((entry & !NODE_FLAG) as usize)
                            .ok_or(Error::InvalidHuffmanTree("table walk out of bounds"))?;
                    }
                    index = index
                        .checked_add(1)
                        .ok_or(Error::InvalidHuffmanTree("table walk out of bounds"))?;
                }
            }
        }
    }
}

/// Decode a serialized small tree (pre-order, one bit per node; leaves
/// carry 8-bit values) into a flattened [`ByteTree`].
///
/// Consumes exactly the tree's bits. Depth is capped at 27 and the leaf
/// count at 256.
pub(crate) fn decode_small_tree(br: &mut BitReader<'_>) -> Result<ByteTree, Error> {
    let mut entries = Vec::new();
    let mut leaves = 0usize;
    build_small_node(br, 0, &mut entries, &mut leaves)?;
    Ok(ByteTree::Tree(entries))
}

/// Decode one node of a small tree, appending to `entries`. Returns the
/// entry count of this subtree (a leaf is 1; an internal node is
/// left + 1 + right), which internal nodes store as the offset to their
/// right subtree.
fn build_small_node(
    br: &mut BitReader<'_>,
    depth: u32,
    entries: &mut Vec<u32>,
    leaves: &mut usize,
) -> Result<u32, Error> {
    if depth > MAX_SMALL_DEPTH {
        return Err(Error::InvalidHuffmanTree("small tree depth exceeded"));
    }
    if br.read_bit()? == 0 {
        if *leaves >= MAX_SMALL_LEAVES {
            return Err(Error::InvalidHuffmanTree("too many leaves"));
        }
        *leaves += 1;
        let value = br.read_bits(8)?;
        entries.push(value);
        return Ok(1);
    }
    let node_index = entries.len();
    entries.push(0); // reserve; patched after the left subtree is known
    let left = build_small_node(br, depth + 1, entries, leaves)?;
    entries[node_index] = NODE_FLAG | left;
    let right = build_small_node(br, depth + 1, entries, leaves)?;
    Ok(left + 1 + right)
}

/// One decoded header tree: the flattened big tree plus the three
/// recency-slot indices. The slots are **positions** in `entries` whose
/// stored values are mutated by the recency cache.
#[derive(Debug, Clone)]
pub struct HeaderTree {
    entries: Vec<u32>,
    slots: [usize; 3],
}

impl HeaderTree {
    /// The degenerate table substituted for an absent tree: a single entry
    /// 0, with all three recency slots at index 1 (the backing store holds
    /// two zero entries so those reads yield 0).
    fn absent() -> Self {
        Self {
            entries: alloc::vec![0, 0],
            slots: [1, 1, 1],
        }
    }

    /// Decode one header tree from the bitstream.
    ///
    /// `size` is the tree's nominal declared size; it bounds the backing
    /// array (`(size + 3) / 4` entries plus 3 extra for late recency
    /// slots) but is not an exact entry count — under-filling is normal.
    fn decode(br: &mut BitReader<'_>, size: u32) -> Result<Self, Error> {
        if size >= u32::MAX / 16 {
            return Err(Error::InvalidHuffmanTree("declared tree size too large"));
        }
        let capacity = size.div_ceil(4) as usize;

        // Two byte-trees (low byte, high byte). A present tree is followed
        // by one unconditionally skipped bit; an absent one is just its
        // flag bit.
        let mut byte_trees = [ByteTree::absent(), ByteTree::absent()];
        for slot in &mut byte_trees {
            if br.read_bit()? != 0 {
                *slot = decode_small_tree(br)?;
                br.skip_bits(1)?;
            }
        }

        // Three 16-bit escape values.
        let mut escapes = [0u32; 3];
        for esc in &mut escapes {
            *esc = br.read_bits(16)?;
        }

        let mut entries = alloc::vec![0u32; capacity + 3];
        let mut ctx = BigTreeCtx {
            entries: &mut entries,
            write: 0,
            capacity,
            escapes,
            slots: [None; 3],
        };
        walk_big_tree(br, 0, &mut ctx, &byte_trees[0], &byte_trees[1])?;
        br.skip_bits(1)?;

        // Slots whose escape value never appeared point past the written
        // entries, one fresh index each.
        let mut next = ctx.write;
        let mut slots = [0usize; 3];
        for (i, slot) in ctx.slots.iter().enumerate() {
            if let Some(idx) = slot {
                slots[i] = *idx;
            } else {
                slots[i] = next;
                next += 1;
            }
        }

        Ok(Self { entries, slots })
    }

    /// Build a tree directly from a flattened table and recency slots, for
    /// unit tests in sibling modules.
    #[cfg(test)]
    pub(crate) fn test_tree(entries: Vec<u32>, slots: [usize; 3]) -> Self {
        Self { entries, slots }
    }

    /// Per-frame reset: write 0 into all three escape slots.
    pub fn reset_cache(&mut self) {
        for &slot in &self.slots {
            if let Some(entry) = self.entries.get_mut(slot) {
                *entry = 0;
            }
        }
    }

    /// Decode one symbol: walk the flattened tree from index 0, then apply
    /// the move-to-front recency update.
    ///
    /// # Errors
    /// Fails if the bitstream runs out mid-walk or the tree is malformed.
    pub fn decode_symbol(&mut self, br: &mut BitReader<'_>) -> Result<u32, Error> {
        let mut index = 0usize;
        loop {
            let entry = *self
                .entries
                .get(index)
                .ok_or(Error::InvalidHuffmanTree("table walk out of bounds"))?;
            if entry & NODE_FLAG == 0 {
                let value = entry;
                self.update_cache(value);
                return Ok(value);
            }
            if br.read_bit()? != 0 {
                index = index
                    .checked_add((entry & !NODE_FLAG) as usize)
                    .ok_or(Error::InvalidHuffmanTree("table walk out of bounds"))?;
            }
            index = index
                .checked_add(1)
                .ok_or(Error::InvalidHuffmanTree("table walk out of bounds"))?;
        }
    }

    /// Move-to-front: if `value` differs from the most-recent slot's
    /// stored value, shift the stack down and store it in slot 0.
    fn update_cache(&mut self, value: u32) {
        let [s0, s1, s2] = self.slots;
        let recent = self.entries.get(s0).copied().unwrap_or(0);
        if recent == value {
            return;
        }
        let v0 = self.entries.get(s0).copied().unwrap_or(0);
        let v1 = self.entries.get(s1).copied().unwrap_or(0);
        if let Some(e) = self.entries.get_mut(s2) {
            *e = v1;
        }
        if let Some(e) = self.entries.get_mut(s1) {
            *e = v0;
        }
        if let Some(e) = self.entries.get_mut(s0) {
            *e = value;
        }
    }
}

/// Mutable state for the recursive big-tree walk.
struct BigTreeCtx<'a> {
    entries: &'a mut [u32],
    write: usize,
    capacity: usize,
    escapes: [u32; 3],
    slots: [Option<usize>; 3],
}

/// Decode one node of a big tree. Returns the leaf count of this subtree,
/// which internal nodes use as the relative offset to their right subtree.
fn walk_big_tree(
    br: &mut BitReader<'_>,
    depth: u32,
    ctx: &mut BigTreeCtx<'_>,
    low: &ByteTree,
    high: &ByteTree,
) -> Result<u32, Error> {
    if depth > MAX_BIG_DEPTH {
        return Err(Error::InvalidHuffmanTree("big tree depth exceeded"));
    }
    if ctx.write >= ctx.capacity {
        return Err(Error::InvalidHuffmanTree("tree size exceeded"));
    }

    if br.read_bit()? == 0 {
        // Leaf: compose a 16-bit value from the two byte-trees.
        let value = u32::from(low.decode(br)?) | (u32::from(high.decode(br)?) << 8);
        // Escape values are checked in order; the slot records the leaf's
        // position and the entry stores 0 instead.
        let mut stored = value;
        for (k, esc) in ctx.escapes.iter().enumerate() {
            if value == *esc {
                ctx.slots[k] = Some(ctx.write);
                stored = 0;
                break;
            }
        }
        *ctx.entries
            .get_mut(ctx.write)
            .ok_or(Error::InvalidHuffmanTree("tree size exceeded"))? = stored;
        ctx.write += 1;
        return Ok(1);
    }

    // Internal node: reserve a slot, then left and right subtrees.
    let node_index = ctx.write;
    ctx.write += 1;
    let left_leaves = walk_big_tree(br, depth + 1, ctx, low, high)?;
    *ctx.entries
        .get_mut(node_index)
        .ok_or(Error::InvalidHuffmanTree("tree size exceeded"))? = NODE_FLAG | left_leaves;
    let right_leaves = walk_big_tree(br, depth + 1, ctx, low, high)?;
    Ok(left_leaves + 1 + right_leaves)
}

/// The four video header trees, in stream order: MMAP, MCLR, FULL, TYPE.
#[derive(Debug, Clone)]
pub struct HuffmanTables {
    /// Mono-block color-pair tree.
    pub mmap: HeaderTree,
    /// Mono-block color tree.
    pub mclr: HeaderTree,
    /// Full-block tree.
    pub full: HeaderTree,
    /// Block-type tree.
    pub type_: HeaderTree,
}

impl HuffmanTables {
    /// Parse the four header trees from the tree bitstream and their
    /// declared sizes (MMAP, MCLR, FULL, TYPE order).
    ///
    /// Each tree starts with a presence flag bit; an absent tree consumes
    /// only that bit and yields a degenerate constant-0 table. Parsing
    /// fails if all four trees are absent or the bitstream is exhausted
    /// exactly after the fourth tree.
    ///
    /// # Errors
    /// Fails on malformed trees, an exhausted bitstream, or all four trees
    /// being absent.
    pub fn parse(tree_data: &[u8], tree_sizes: [u32; 4]) -> Result<Self, Error> {
        let mut br = BitReader::new(tree_data);
        let mut trees = Vec::with_capacity(4);
        for size in tree_sizes {
            if br.read_bit()? != 0 {
                trees.push(HeaderTree::decode(&mut br, size)?);
            } else {
                trees.push(HeaderTree::absent());
            }
        }
        if trees
            .iter()
            .all(|t| t.entries.len() == 2 && t.slots == [1, 1, 1])
        {
            return Err(Error::NoTreesPresent);
        }
        if br.bits_remaining() == 0 {
            return Err(Error::InvalidHuffmanTree("bitstream exhausted after trees"));
        }

        let mut iter = trees.into_iter();
        let (Some(mmap), Some(mclr), Some(full), Some(type_)) =
            (iter.next(), iter.next(), iter.next(), iter.next())
        else {
            return Err(Error::NoTreesPresent);
        };
        Ok(Self {
            mmap,
            mclr,
            full,
            type_,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// LSB-first bit writer for hand-built test streams.
    struct BitWriter {
        bytes: Vec<u8>,
        pos: usize,
    }

    impl BitWriter {
        fn new() -> Self {
            Self {
                bytes: Vec::new(),
                pos: 0,
            }
        }

        fn push_bit(&mut self, bit: bool) {
            if self.pos.is_multiple_of(8) {
                self.bytes.push(0);
            }
            if bit {
                let last = self.bytes.last_mut().unwrap();
                *last |= 1 << (self.pos % 8);
            }
            self.pos += 1;
        }

        fn push_bits(&mut self, value: u32, n: u32) {
            for i in 0..n {
                self.push_bit(value & (1 << i) != 0);
            }
        }

        fn append(&mut self, other: &Self) {
            for i in 0..other.pos {
                self.push_bit(other.bytes[i / 8] & (1 << (i % 8)) != 0);
            }
        }

        fn finish(self) -> Vec<u8> {
            self.bytes
        }
    }

    /// Serialize a small tree: root internal node, left leaf `a`, right
    /// leaf `b`. Codes: `a` = 0, `b` = 1 (both length 1).
    fn two_leaf_tree_bits(w: &mut BitWriter, a: u8, b: u8) {
        w.push_bit(true); // internal
        w.push_bit(false); // left leaf
        w.push_bits(u32::from(a), 8);
        w.push_bit(false); // right leaf
        w.push_bits(u32::from(b), 8);
    }

    #[test]
    fn small_tree_two_leaves() {
        let mut w = BitWriter::new();
        two_leaf_tree_bits(&mut w, 0x41, 0x42);
        // Code bits to decode after the tree: 0, 1, 0.
        w.push_bit(false);
        w.push_bit(true);
        w.push_bit(false);
        let data = w.finish();

        let mut br = BitReader::new(&data);
        let tree = decode_small_tree(&mut br).unwrap();
        // 'A' has code 0, 'B' has code 1.
        assert_eq!(tree.decode(&mut br).unwrap(), 0x41);
        assert_eq!(tree.decode(&mut br).unwrap(), 0x42);
        assert_eq!(tree.decode(&mut br).unwrap(), 0x41);
    }

    #[test]
    fn small_tree_single_leaf_is_constant() {
        let mut w = BitWriter::new();
        w.push_bit(false); // root is a leaf
        w.push_bits(0x77, 8);
        w.push_bits(0xFF, 8); // trailing garbage must not be consumed
        let data = w.finish();

        let mut br = BitReader::new(&data);
        let tree = decode_small_tree(&mut br).unwrap();
        let before = br.position();
        assert_eq!(tree.decode(&mut br).unwrap(), 0x77);
        assert_eq!(tree.decode(&mut br).unwrap(), 0x77);
        assert_eq!(br.position(), before, "constant tree consumed bits");
    }

    #[test]
    fn small_tree_depth_limit() {
        // 30 nested internal nodes with no leaves.
        let mut w = BitWriter::new();
        for _ in 0..30 {
            w.push_bit(true);
        }
        let data = w.finish();
        let mut br = BitReader::new(&data);
        assert!(matches!(
            decode_small_tree(&mut br),
            Err(Error::InvalidHuffmanTree(_))
        ));
    }

    #[test]
    fn small_tree_noncanonical_paths() {
        // Left subtree has two depth-2 leaves (codes 00, 01), right
        // subtree is a depth-1 leaf (code 1). Decoding must follow the
        // tree paths; a canonical length-sorted assignment would wrongly
        // give the depth-1 leaf code 0.
        let mut w = BitWriter::new();
        w.push_bit(true); // root internal
        w.push_bit(true); // left internal
        w.push_bit(false); // leaf A = 00
        w.push_bits(0x41, 8);
        w.push_bit(false); // leaf B = 01
        w.push_bits(0x42, 8);
        w.push_bit(false); // leaf C = 1
        w.push_bits(0x43, 8);
        // Code bits: 1 (C), 00 (A), 01 (B).
        w.push_bit(true);
        w.push_bit(false);
        w.push_bit(false);
        w.push_bit(false);
        w.push_bit(true);
        let data = w.finish();

        let mut br = BitReader::new(&data);
        let tree = decode_small_tree(&mut br).unwrap();
        assert_eq!(tree.decode(&mut br).unwrap(), 0x43);
        assert_eq!(tree.decode(&mut br).unwrap(), 0x41);
        assert_eq!(tree.decode(&mut br).unwrap(), 0x42);
    }

    /// Build a header-tree bitstream: low byte-tree present with leaves
    /// 0x10 (code 0) and 0x05 (code 1), high byte-tree absent, escapes
    /// [0x0010, 0x2000, 0x3000], and a big tree of one internal node with
    /// two leaves (escape leaf on the left, plain leaf on the right).
    fn sample_header_tree() -> (BitWriter, u32) {
        let mut w = BitWriter::new();
        w.push_bit(true); // low byte-tree present
        two_leaf_tree_bits(&mut w, 0x10, 0x05);
        w.push_bit(false); // unconditional skip bit after the tree
        w.push_bit(false); // high byte-tree absent
        for esc in [0x0010u32, 0x2000, 0x3000] {
            w.push_bits(esc, 16);
        }
        // Big tree: internal node, left leaf (value 0x10 == escape 0),
        // right leaf (value 0x05).
        w.push_bit(true);
        w.push_bit(false);
        w.push_bit(false); // low byte code 0 -> 0x10
        w.push_bit(false);
        w.push_bit(true); // low byte code 1 -> 0x05
        w.push_bit(true); // skip bit after the big tree
        // Declared size: 3 entries written -> nominal 12 bytes; declare
        // more to prove under-fill is tolerated.
        (w, 64)
    }

    #[test]
    fn header_tree_decode_walk_and_recency() {
        let (w, size) = sample_header_tree();
        let data = w.finish();
        let mut br = BitReader::new(&data);
        let mut tree = HeaderTree::decode(&mut br, size).unwrap();

        // Layout: index 0 = node (offset 1), 1 = escape slot 0 (value 0),
        // 2 = plain leaf 0x0005. Slots 1 and 2 point past written entries.
        assert_eq!(tree.entries[0], NODE_FLAG | 1);
        assert_eq!(tree.entries[1], 0);
        assert_eq!(tree.entries[2], 0x0005);
        assert_eq!(tree.slots[0], 1);
        assert_eq!(tree.slots[1], 3);
        assert_eq!(tree.slots[2], 4);

        // Walk: bit 0 -> escape slot (value 0); bit 1 -> 0x0005.
        let mut bits = BitWriter::new();
        bits.push_bit(false);
        bits.push_bit(true);
        let bits = bits.finish();
        let mut br = BitReader::new(&bits);
        assert_eq!(tree.decode_symbol(&mut br).unwrap(), 0);
        // Decoding 0 equals the most-recent slot (0): no cache change.
        assert_eq!(tree.entries[1], 0);
        assert_eq!(tree.decode_symbol(&mut br).unwrap(), 5);
        // 5 differs from most-recent (0): shift down, store in slot 0.
        assert_eq!(tree.entries[tree.slots[0]], 5);
        assert_eq!(tree.entries[tree.slots[1]], 0);
        assert_eq!(tree.entries[tree.slots[2]], 0);

        // Per-frame reset clears the slots.
        tree.reset_cache();
        assert_eq!(tree.entries[tree.slots[0]], 0);
    }

    #[test]
    fn header_tree_underfill_is_tolerated() {
        let (w, _size) = sample_header_tree();
        let data = w.finish();
        let mut br = BitReader::new(&data);
        // Absurdly large declared size: still fine.
        let tree = HeaderTree::decode(&mut br, 4096).unwrap();
        assert_eq!(tree.entries.len(), 4096 / 4 + 3);
    }

    #[test]
    fn header_tree_overflow_rejected() {
        let (w, _size) = sample_header_tree();
        let data = w.finish();
        let mut br = BitReader::new(&data);
        // Declared size 4 -> capacity 1: the internal node fits but its
        // children do not.
        assert!(matches!(
            HeaderTree::decode(&mut br, 4),
            Err(Error::InvalidHuffmanTree(_))
        ));
    }

    #[test]
    fn absent_tree_consumes_only_its_flag_bit() {
        // Four absent trees followed by a trailing 1 bit.
        let mut w = BitWriter::new();
        for _ in 0..4 {
            w.push_bit(false);
        }
        w.push_bit(true);
        let data = w.finish();

        assert!(matches!(
            HuffmanTables::parse(&data, [0, 0, 0, 0]),
            Err(Error::NoTreesPresent)
        ));

        // First tree present (minimal), rest absent: the absent trees must
        // not swallow the following bits.
        let (tree_bits, size) = sample_header_tree();
        let mut w = BitWriter::new();
        w.push_bit(true); // MMAP present
        w.append(&tree_bits);
        w.push_bit(false); // MCLR absent
        w.push_bit(false); // FULL absent
        w.push_bit(false); // TYPE absent
        w.push_bit(true); // trailing bit so the stream isn't exhausted
        let data = w.finish();
        let tables = HuffmanTables::parse(&data, [size, 0, 0, 0]).unwrap();
        assert_eq!(tables.mmap.entries[2], 0x0005);
        // Absent trees decode to 0 without consuming bits.
        let mut br = BitReader::new(&[0xFF]);
        assert_eq!(tables.mclr.clone().decode_symbol(&mut br).unwrap(), 0);
        assert_eq!(br.position(), 0);
    }

    #[test]
    fn exhausted_after_trees_fails() {
        // One present tree whose total length makes the four trees consume
        // the stream exactly: presence(1) + low tree(1+1+8+1) + high
        // absent(1) + escapes(48) + big tree(7) + skip(1) + 3 absent
        // flags = 72 bits = 9 bytes.
        let mut w = BitWriter::new();
        w.push_bit(true); // MMAP present
        w.push_bit(true); // low byte-tree present
        w.push_bit(false); // single leaf
        w.push_bits(0x10, 8);
        w.push_bit(false); // skip bit
        w.push_bit(false); // high byte-tree absent
        for esc in [0x2000u32, 0x3000, 0x4000] {
            w.push_bits(esc, 16);
        }
        // Big tree with 4 leaves (7 bits): root internal, two internal
        // children, two leaves each.
        w.push_bit(true);
        w.push_bit(true);
        w.push_bit(false);
        w.push_bit(false);
        w.push_bit(true);
        w.push_bit(false);
        w.push_bit(false);
        w.push_bit(true); // skip bit
        w.push_bit(false); // MCLR absent
        w.push_bit(false); // FULL absent
        w.push_bit(false); // TYPE absent
        let data = w.finish();
        assert_eq!(data.len(), 9);

        assert!(matches!(
            HuffmanTables::parse(&data, [64, 0, 0, 0]),
            Err(Error::InvalidHuffmanTree(_))
        ));
    }

    #[test]
    fn big_tree_last_escape_occurrence_wins() {
        // Big tree: internal root; left subtree internal with two escape
        // leaves; right leaf plain. Slot 0 must point at the *last* leaf
        // whose value equals escape 0.
        let mut w = BitWriter::new();
        w.push_bit(true); // low byte-tree present
        two_leaf_tree_bits(&mut w, 0x10, 0x05);
        w.push_bit(false); // skip bit
        w.push_bit(false); // high byte-tree absent
        for esc in [0x0010u32, 0x2000, 0x3000] {
            w.push_bits(esc, 16);
        }
        w.push_bit(true); // root internal (index 0)
        w.push_bit(true); // left internal (index 1)
        w.push_bit(false);
        w.push_bit(false); // leaf 0x10 at index 2
        w.push_bit(false);
        w.push_bit(false); // leaf 0x10 at index 3
        w.push_bit(false);
        w.push_bit(true); // leaf 0x05 at index 4
        w.push_bit(true); // skip bit
        let data = w.finish();

        let mut br = BitReader::new(&data);
        let tree = HeaderTree::decode(&mut br, 64).unwrap();
        assert_eq!(tree.slots[0], 3, "slot points at the last escape leaf");
        assert_eq!(tree.entries[2], 0);
        assert_eq!(tree.entries[3], 0);
        // Root's right-subtree offset: the left subtree occupies 3
        // entries (one node + two leaves), so the right child of the root
        // is at index 1 + 3 = 4.
        assert_eq!(tree.entries[0], NODE_FLAG | 3);
        assert_eq!(tree.entries[1], NODE_FLAG | 1);
    }
}
