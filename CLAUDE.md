# Claude Code Notes

This repository already contains reusable AI helper guidance under `.github/skills/`.
Claude Code users should follow the same conventions when suggesting branch names,
commit messages, and release versions.

## Branch naming

- Use lowercase conventional branch names like `feature/...`, `fix/...`, `chore/...`, or `release/...`.
- Use hyphen-separated descriptions.
- Keep branch names short and purpose-driven.

Examples:

- `feature/add-openwrt-ci-cache`
- `fix/kuiper-driver-build`
- `chore/update-agent-skills`

See `.github/skills/conventional-branch/SKILL.md` for the full convention.

## Commit messages

- Use Conventional Commits.
- Prefer forms like `feat: ...`, `fix: ...`, `ci: ...`, `build: ...`, `docs: ...`, or `chore: ...`.
- Add a scope when it improves clarity, for example `fix(driver): ...`.

Examples:

- `ci: add Kuiper build workflow`
- `fix(driver): keep legacy ieee80211_ops signatures for adi builds`
- `docs(openwrt): update build instructions`

See `.github/skills/conventional-commits/SKILL.md` for the full convention.

## Versioning

- Use Semantic Versioning for releases and release planning.
- Tag names may use a leading `v`, but the version itself should remain `MAJOR.MINOR.PATCH`.

Examples:

- `1.5.0`
- `2.0.0-rc.1`
- `git tag v1.5.0`

See `.github/skills/semantic-versioning/SKILL.md` for the full convention.