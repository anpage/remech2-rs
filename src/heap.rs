use std::{
    alloc::{self, Layout},
    collections::HashMap,
    ffi::c_void,
    ptr,
};

use tracing::warn;

const ALIGN: usize = 16;

#[derive(Default)]
pub struct MechHeap {
    blocks: HashMap<*mut u8, Layout>,
}

impl MechHeap {
    fn alloc(&mut self, size: usize, zeroed: bool) -> *mut u8 {
        let Some(layout) = layout(size) else {
            return ptr::null_mut();
        };

        let block = unsafe {
            if zeroed {
                alloc::alloc_zeroed(layout)
            } else {
                alloc::alloc(layout)
            }
        };
        if !block.is_null() {
            self.blocks.insert(block, layout);
        }
        block
    }

    fn realloc(&mut self, block: *mut u8, size: usize) -> *mut u8 {
        let (Some(old), Some(new)) = (self.blocks.get(&block).copied(), layout(size)) else {
            return ptr::null_mut();
        };

        let moved = unsafe { alloc::realloc(block, old, new.size()) };
        if !moved.is_null() {
            self.blocks.remove(&block);
            self.blocks.insert(moved, new);
        }
        moved
    }

    fn free(&mut self, block: *mut u8) -> bool {
        let Some(layout) = self.blocks.remove(&block) else {
            return false;
        };

        unsafe { alloc::dealloc(block, layout) };
        true
    }
}

impl Drop for MechHeap {
    fn drop(&mut self) {
        for (block, layout) in self.blocks.drain() {
            unsafe { alloc::dealloc(block, layout) };
        }
    }
}

fn layout(size: usize) -> Option<Layout> {
    Layout::from_size_align(size.max(1), ALIGN).ok()
}

#[unsafe(export_name = "MechHeapCreate")]
pub extern "C" fn create() -> *mut MechHeap {
    Box::into_raw(Box::default())
}

#[unsafe(export_name = "MechHeapDestroy")]
pub unsafe extern "C" fn destroy(heap: *mut MechHeap) {
    if !heap.is_null() {
        drop(unsafe { Box::from_raw(heap) });
    }
}

#[unsafe(export_name = "MechHeapAlloc")]
pub unsafe extern "C" fn alloc(heap: *mut MechHeap, size: usize) -> *mut c_void {
    let Some(heap) = (unsafe { heap.as_mut() }) else {
        return ptr::null_mut();
    };
    heap.alloc(size, false).cast()
}

#[unsafe(export_name = "MechHeapAllocZeroed")]
pub unsafe extern "C" fn alloc_zeroed(heap: *mut MechHeap, size: usize) -> *mut c_void {
    let Some(heap) = (unsafe { heap.as_mut() }) else {
        return ptr::null_mut();
    };
    heap.alloc(size, true).cast()
}

#[unsafe(export_name = "MechHeapReAlloc")]
pub unsafe extern "C" fn realloc(
    heap: *mut MechHeap,
    block: *mut c_void,
    size: usize,
) -> *mut c_void {
    let Some(heap) = (unsafe { heap.as_mut() }) else {
        return ptr::null_mut();
    };
    heap.realloc(block.cast(), size).cast()
}

#[unsafe(export_name = "MechHeapFree")]
pub unsafe extern "C" fn free(heap: *mut MechHeap, block: *mut c_void) -> i32 {
    let Some(heap) = (unsafe { heap.as_mut() }) else {
        return 0;
    };

    let freed = heap.free(block.cast());
    if !freed && !block.is_null() {
        warn!("heap: ignored free of {block:p}, which isn't allocated");
    }
    i32::from(freed)
}

#[unsafe(export_name = "MechHeapSize")]
pub unsafe extern "C" fn size(heap: *mut MechHeap, block: *const c_void) -> usize {
    let Some(heap) = (unsafe { heap.as_ref() }) else {
        return usize::MAX;
    };
    heap.blocks
        .get(&block.cast_mut().cast())
        .map_or(usize::MAX, |layout| layout.size())
}
