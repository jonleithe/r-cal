# Project History

This is a concise, chronological record of major milestones and decisions for
`r-cal`. It is not a substitute for Git history or a release changelog.

## 2026-11-04

### C and C++ selected

- The project will be implemented in C and C++.
- The compiler, C and C++ language standards, build system, and test tooling
  remain to be selected.

### Repository initialized

- Created the repository with an MIT license, a starter `.gitignore`, and a
  minimal `README.md`.
- Initial commit: `a01cc1b` (`Initial commit`).

### Shared project context added

- Added `AGENTS.md` to keep working conventions consistent across computers and
  contributors.
- Added this history file so important decisions and milestones remain easy to
  find without reconstructing them from individual commits.
- The project is still in its bootstrap phase; its scope, architecture, and
  development commands have not yet been documented.

## Entry format

Add new entries in reverse chronological order using this structure:

```md
## YYYY-MM-DD

### Short milestone or decision title

- What changed.
- Why it changed, when that context will help future contributors.
- Relevant issue, pull request, release, or commit reference, if available.
```

Record durable context rather than a list of every file changed. Correct older
entries only when they are factually wrong; otherwise add a new dated entry so
the evolution of the project remains visible.
