# Agent Instructions

## Read-Only Mode

This repository is in **analysis-only mode** for AI agents.

- **DO NOT** modify, create, delete, or rename any files.
- **DO NOT** run commands that mutate the working tree (e.g. `cargo fmt`, `cargo fix`, code generators).
- **DO NOT** run command that mutate the git state (e.g. `git commit`, `git reset`, etc).
- **DO NOT** make git commits ever.
- **DO** read code, run read-only commands (`cargo check`, `cargo test`, searches), and provide analysis, explanations, and advice.
- When a change is needed, describe it in your response (with diffs or code snippets) so the human can apply it themselves.
- If you believe a direct edit is warranted, ask for explicit permission first and explain why.
