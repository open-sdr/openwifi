---
name: semantic-versioning
description: 'Apply and validate Semantic Versioning 2.0.0 (SemVer). Use when choosing the next version number, deciding a MAJOR/MINOR/PATCH bump, releasing or tagging (git tag vX.Y.Z), adding pre-release (-alpha.1, -rc.1) or build metadata (+build.5), comparing version precedence/ordering, or checking whether a version string is valid. Produces MAJOR.MINOR.PATCH versions like 1.4.2, 2.0.0-rc.1, or 1.0.0+20130313144700.'
argument-hint: 'describe the change (e.g. "added a backward-compatible endpoint") or a version to validate'
---

# Semantic Versioning (SemVer 2.0.0)

Assign and compare version numbers following the [Semantic Versioning 2.0.0](https://semver.org/) specification, so version changes convey precise meaning about API compatibility.

## When to Use

- Deciding the next version number after a set of changes.
- Choosing whether a change is a MAJOR, MINOR, or PATCH bump.
- Tagging a release (`git tag vX.Y.Z`).
- Adding pre-release identifiers or build metadata.
- Comparing or ordering versions by precedence.
- Validating a version string.

**Prerequisite:** SemVer requires a declared public API (in code or docs). Bumps are defined relative to changes in that API.

## Format

```
MAJOR.MINOR.PATCH[-prerelease][+buildmetadata]
```

Each of MAJOR, MINOR, PATCH is a non-negative integer with no leading zeroes (e.g. `1.10.0`, never `1.010.0`). Once released, a version's contents MUST NOT change — release a new version instead.

## Procedure

1. **Identify the highest-impact change** since the last release.
2. **Pick the bump** using the decision table below.
3. **Reset lower parts to 0** when bumping a higher part (MINOR reset resets PATCH; MAJOR reset resets both MINOR and PATCH).
4. **Append pre-release / build metadata** if needed (see rules below).
5. **Validate** the string against the regex, then **tag the release**:
   ```bash
   git tag -a v1.4.0 -m "Release 1.4.0"
   git push origin v1.4.0
   ```
   Note: the `v` prefix is a tag-name convention only — the semantic version itself is `1.4.0`, not `v1.4.0`.

## Choosing the Bump

Given a current version `X.Y.Z` (with `X > 0`):

| Change to the public API | Bump | Example |
|--------------------------|------|---------|
| Backward-**incompatible** change (breaking) | MAJOR → `(X+1).0.0` | `1.4.2` → `2.0.0` |
| Backward-**compatible** new functionality | MINOR → `X.(Y+1).0` | `1.4.2` → `1.5.0` |
| Marking any functionality as **deprecated** | MINOR | `1.4.2` → `1.5.0` |
| Backward-compatible **bug fix** only | PATCH → `X.Y.(Z+1)` | `1.4.2` → `1.4.3` |

Rules:
- MAJOR MUST be incremented for any breaking API change; MAY also include minor/patch changes; MINOR and PATCH reset to 0.
- MINOR MUST be incremented for backward-compatible additions or when marking functionality deprecated; MAY be incremented for substantial internal improvements; PATCH resets to 0.
- PATCH MUST be incremented only for backward-compatible bug fixes (internal changes that fix incorrect behavior).

## Initial Development (0.y.z)

- Major version zero (`0.y.z`) is for initial development — anything MAY change at any time; the public API SHOULD NOT be considered stable.
- Start at `0.1.0` and bump MINOR for each subsequent release.
- Release `1.0.0` once the API is stable / used in production.

## Pre-release Versions

Append a hyphen and dot-separated identifiers after PATCH:

- Identifiers MUST be ASCII alphanumerics and hyphens `[0-9A-Za-z-]`, MUST NOT be empty.
- Numeric identifiers MUST NOT include leading zeroes.
- A pre-release has **lower** precedence than its associated normal version (`1.0.0-alpha` < `1.0.0`).
- Examples: `1.0.0-alpha`, `1.0.0-alpha.1`, `1.0.0-0.3.7`, `1.0.0-x.7.z.92`, `1.0.0-rc.1`.

## Build Metadata

Append a plus sign and dot-separated identifiers after PATCH or pre-release:

- Same character set `[0-9A-Za-z-]`; identifiers MUST NOT be empty.
- Build metadata MUST be **ignored** when determining precedence (two versions differing only in build metadata rank equal).
- Examples: `1.0.0-alpha+001`, `1.0.0+20130313144700`, `1.0.0-beta+exp.sha.5114f85`.

## Precedence (Ordering)

Compare MAJOR, then MINOR, then PATCH numerically. Build metadata is ignored. When core versions are equal, a pre-release ranks lower than the normal version. Between two pre-releases, compare each dot-separated identifier left to right:

1. Numeric-only identifiers compare numerically.
2. Identifiers with letters/hyphens compare lexically in ASCII order.
3. Numeric identifiers always rank lower than non-numeric identifiers.
4. A larger set of pre-release fields ranks higher if all preceding identifiers are equal.

Worked example:

```
1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta
  < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
```

And core ordering: `1.0.0 < 2.0.0 < 2.1.0 < 2.1.1`.

## Validation

Official SemVer regex (ECMAScript / PCRE / Python / Go compatible; capture groups: 1=major, 2=minor, 3=patch, 4=prerelease, 5=buildmetadata):

```
^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$
```

Quick shell check (GNU grep, PCRE mode):

```bash
ver="1.4.2-alpha.1+build.5"
echo "$ver" | grep -Pq '^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$' \
  && echo valid || echo invalid
```

## Examples

| Version | Valid | Note |
|---------|:-----:|------|
| `1.0.0` | ✅ | Normal release |
| `0.1.0` | ✅ | Initial development |
| `1.10.0` | ✅ | Multi-digit, no leading zero |
| `2.0.0-rc.1` | ✅ | Release candidate pre-release |
| `1.0.0-alpha.1` | ✅ | Pre-release |
| `1.0.0-0.3.7` | ✅ | Numeric pre-release identifiers |
| `1.0.0+20130313144700` | ✅ | Build metadata only |
| `1.0.0-beta+exp.sha.5114f85` | ✅ | Pre-release + build metadata |
| `v1.2.3` | ❌ | `v` is a tag prefix, not part of the version |
| `1.2` | ❌ | Missing PATCH |
| `1.02.3` | ❌ | Leading zero not allowed |
| `1.0.0-alpha..1` | ❌ | Empty pre-release identifier |
| `1.0.0-alpha_beta` | ❌ | Underscore not allowed |

## Key FAQ Points

- **Is `v1.2.3` a semantic version?** No. Prefixing with `v` is a common tag-name convention; the version is `1.2.3`.
- **When is `1.0.0`?** When the software has a stable public API or is used in production.
- **Accidentally shipped a breaking change as MINOR/PATCH?** Do not modify the released version; release a new version that corrects it, and document the offending version.
- **Deprecations?** Ship a MINOR release that marks the API deprecated (and document it) before removing it in a later MAJOR release.
