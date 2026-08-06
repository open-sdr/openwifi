---
name: conventional-commits
description: 'Write Git commit messages that follow the Conventional Commits 1.0.0 specification. Use when composing a commit message, running git commit, choosing a commit type (feat/fix/docs/refactor/ci/chore/etc.), adding a scope, marking a BREAKING CHANGE, writing commit bodies/footers, or checking a message against the spec. Produces messages like "feat(parser): add array support" that map cleanly to SemVer bumps.'
argument-hint: 'describe the change (e.g. "fixed a null deref in the parser") to get a commit message'
---

# Conventional Commits 1.0.0

Write commit messages following the [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/) specification so history is explicit and drives SemVer bumps, changelogs, and release automation.

## When to Use

- Composing any commit message (`git commit`).
- Choosing the right type/scope for a change.
- Indicating a breaking change.
- Writing a multi-paragraph body or footers (e.g. `Refs:`, `Reviewed-by:`).
- Validating an existing message against the spec.

## Format

```
<type>[optional scope][!]: <description>

[optional body]

[optional footer(s)]
```

- **type**: a noun such as `feat` or `fix`, followed by an optional scope, an optional `!`, then a REQUIRED colon and space.
- **description**: a short summary, immediately after the `: `.
- **body**: optional; begins **one blank line** after the description; free-form paragraphs.
- **footer(s)**: optional; begin **one blank line** after the body; each is `Token: value` or `Token #value`.

## Procedure

1. **Pick the type** that best matches the change (see table). If it fits more than one, make separate commits.
2. **Add a scope** if it usefully names the affected area: `fix(driver): ...`.
3. **Write the description**: imperative mood, concise, lowercase, no trailing period.
4. **Mark breaking changes** with `!` before the colon and/or a `BREAKING CHANGE:` footer.
5. **Add body/footers** if context, reasoning, or references are needed.
6. **Commit**, preferably passing the message via `-m` (repeat `-m` for body/footer paragraphs):
   ```bash
   git commit -m "feat(parser): add ability to parse arrays"
   # with body + footer:
   git commit -m "fix: prevent racing of requests" \
     -m "Introduce a request id and a reference to the latest request." \
     -m "Refs: #123"
   ```

## Types (and SemVer mapping)

| Type | Use for | SemVer |
|------|---------|:------:|
| `feat` | A new feature | MINOR |
| `fix` | A bug fix | PATCH |
| `docs` | Documentation only | — |
| `style` | Formatting/whitespace, no code-behavior change | — |
| `refactor` | Code change that neither fixes a bug nor adds a feature | — |
| `perf` | Performance improvement | — |
| `test` | Adding or fixing tests | — |
| `build` | Build system or external dependencies | — |
| `ci` | CI configuration and pipelines | — |
| `chore` | Other maintenance (tooling, housekeeping) | — |
| `revert` | Revert a previous commit | — |
| any type with `!` or `BREAKING CHANGE:` | Backward-incompatible change | MAJOR |

Only `feat` and `fix` are mandated by the spec; the rest are the widely used Angular/commitlint set. Types other than `feat`/`fix` have no implicit SemVer effect unless they carry a breaking change. Teams MAY define additional types.

## Breaking Changes

Indicate a breaking change in **either** place (or both):

- **In the prefix**: put `!` immediately before the colon — `feat!: ...` or `feat(api)!: ...`. The description then describes the break, and the footer MAY be omitted.
- **In a footer**: `BREAKING CHANGE: <description>`. The token MUST be uppercase (`BREAKING-CHANGE` is an accepted synonym). A breaking change may accompany any type.

```
feat!: drop support for Node 6

BREAKING CHANGE: use JavaScript features not available in Node 6.
```

## Scope

A scope is a noun in parentheses naming a section of the codebase: `feat(lang): add Polish language`, `fix(parser): handle empty input`. It is optional.

## Body & Footers

- The body begins one blank line after the description and may span multiple paragraphs.
- Footers begin one blank line after the body. Each footer token uses `-` instead of spaces (e.g. `Reviewed-by`, `Acked-by`, `Refs`), except `BREAKING CHANGE`. Separator is `: ` or ` #`.
- A footer value may contain spaces and newlines; parsing ends at the next valid footer token.

```
fix: prevent racing of requests

Introduce a request id and a reference to latest request. Dismiss
incoming responses other than from latest request.

Reviewed-by: Z
Refs: #123
```

## Reverts

The spec does not mandate revert behavior. Recommended form: the `revert` type with a footer referencing the reverted commit SHAs.

```
revert: let us never again speak of the noodle incident

Refs: 676104e, a215868
```

## Casing

Units of a Conventional Commit are case-insensitive, **except** `BREAKING CHANGE`, which MUST be uppercase. Pick one casing for types and be consistent (this repo uses lowercase, e.g. `fix:`, `ci:`).

## Examples

| Message | Valid | Note |
|---------|:-----:|------|
| `docs: correct spelling of CHANGELOG` | ✅ | No body |
| `feat(lang): add Polish language` | ✅ | Scope |
| `feat: allow config object to extend other configs` | ✅ | With `BREAKING CHANGE:` footer |
| `feat(api)!: send email when a product is shipped` | ✅ | Scope + `!` breaking |
| `revert: let us never again speak of the noodle incident` | ✅ | Revert with `Refs:` footer |
| `fix: array parsing issue when multiple spaces in string` | ✅ | Imperative description |
| `feat:missing space after colon` | ❌ | Colon MUST be followed by a space |
| `Fixed the parser bug` | ❌ | No type prefix |
| `feat(parser) add arrays` | ❌ | Missing colon |
| `feat: breaking change` (real break, no `!`/footer) | ❌ | Breaking change not indicated |

## Why It Matters

Conventional Commits enable automatic CHANGELOG generation, automatic SemVer bump detection (`fix`→PATCH, `feat`→MINOR, breaking→MAJOR), clearer history, and triggering of build/publish pipelines. It pairs with the `conventional-branch` (branch names) and `semantic-versioning` (release versions) skills.
