# CLAUDE.md

General agent guidance for this repository lives in `AGENTS.md`, the single
source of truth for all coding agents. It is imported below.

@AGENTS.md

## Claude Code specifics

- `AGENTS.md` above is the whole contract: follow those conventions whenever the
  user asks you to branch, commit, tag, or open a pull request.
- The detailed, conditionally loaded convention skills currently live under
  `.github/skills/` for GitHub Copilot. Their essentials are always-on here via
  the `AGENTS.md` import. To expose them to Claude Code as conditional skills,
  mirror them under `.claude/skills/`.