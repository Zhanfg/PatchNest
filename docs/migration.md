# Monorepo migration

Canonical repository: `Zhanfg/PatchNest`.

## Imported histories

- CLI: original PatchNest history, relocated to `cli/`.
- Module/WebUI: non-squashed subtree import from `Zhanfg/PatchNest-Module`.
- KPM catalog/source: non-squashed subtree import from `Zhanfg/PatchNest-Kpms`.
- KernelPatch-Public: intentionally remains an independent repository and dependency.

Exact source commits and source-ref inventory are recorded in `docs/provenance/`.

## URL compatibility

Runtime metadata is migrated only where the canonical target exists after the monorepo merge:

- Module `updateJson` → `PatchNest/main/module/update.json`.
- Module changelog → `PatchNest/main/module/CHANGELOG.md`.
- Official KPM catalog → `PatchNest/main/kpms/kpm_repo.json`.
- The old PatchNest-Kpms raw catalog is recognized by the WebUI as a retired URL and automatically normalized to the canonical catalog.

The current `module/update.json` ZIP URL intentionally remains on the immutable standalone `PatchNest-Module` release because that exact asset already exists and its SHA-256 is pinned. It must not be switched to a canonical URL until an equivalent canonical `module-v<version>` release asset has been published and verified.

## Source repositories

The source repositories are not deleted. Historical releases, issue/PR discussions, and unmerged development branches remain available there. Archive readiness is evaluated separately after active branch/PR work and the release-asset cutover are resolved.
