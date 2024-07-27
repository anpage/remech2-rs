use anyhow::{bail, Result};
use hex_literal::hex;
use sha2::{Digest, Sha256};

use super::{Action, Stage};

const SIM_DLL_HASH: [u8; 32] =
    hex!("6212d542f8f915a594b278ab189f20a27e522e7c08ac57ce68bf47f45b17bbb5");

const SHELL_DLL_HASH: [u8; 32] =
    hex!("1078ccb07fd45388bfd525719a8206ce7a01d6536776d30a5655ccb928900879");

const AIL_DLL_HASH: [u8; 32] =
    hex!("2a2134551ad7cc20f66172571b07d1650703db389b30dc6543fff1da1a761d85");

pub struct DllCheck;

impl Stage for DllCheck {
    fn ui(&mut self, _ctx: &egui::Context) -> Result<Action> {
        // TODO: Show problems in the UI

        let sim_dll_hash = {
            let mut hasher = Sha256::new();
            let file = std::fs::read("MW2.DLL")?;
            hasher.update(&file);
            hasher.finalize()
        };

        if sim_dll_hash.as_slice() != SIM_DLL_HASH {
            bail!("MW2.DLL hash mismatch");
        }

        let shell_dll_hash = {
            let mut hasher = Sha256::new();
            let file = std::fs::read("MW2SHELL.DLL")?;
            hasher.update(&file);
            hasher.finalize()
        };

        if shell_dll_hash.as_slice() != SHELL_DLL_HASH {
            bail!("MW2SHELL.DLL hash mismatch");
        }

        let ail_dll_hash = {
            let mut hasher = Sha256::new();
            let file = std::fs::read("WAIL32.DLL")?;
            hasher.update(&file);
            hasher.finalize()
        };

        if ail_dll_hash.as_slice() != AIL_DLL_HASH {
            bail!("WAIL32.DLL hash mismatch");
        }

        Ok(Action::Break)
    }
}
