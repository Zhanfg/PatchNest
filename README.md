# PatchNest

PatchNest is the canonical monorepo for the PatchNest userspace CLI, root module/WebUI, and KPM catalog/source workspace.

## Components

| Path | Component | Release cadence |
|---|---|---|
| `cli/` | Android `kpatch` userspace CLI | Independent |
| `module/` | Magisk / KernelSU / KernelSU-Next / APatch module and WebUI | Independent |
| `kpms/` | KPM catalog, source prototypes, and validation | Independent |
| `docs/` | Monorepo provenance, migration, and release documentation | Repository-wide |

`Zhanfg/KernelPatch-Public` remains an independent lower-level dependency. It is intentionally not vendored into this repository.

## History and migration

The CLI history is the original history of this repository, relocated under `cli/`.

`PatchNest-Module` and `PatchNest-Kpms` are imported with non-squashed Git subtree merges so their source main histories remain reachable from this repository. Exact source refs are recorded under `docs/provenance/`.

Historical releases in the source repositories remain immutable. Public update URLs are migrated only when an equivalent canonical release or raw path exists; compatibility must not be broken merely to complete the repository consolidation.

## Licensing

This monorepo contains components under different licenses. Do not treat the repository as having one blanket license.

See [`docs/licensing.md`](docs/licensing.md) and the component-local license files.

## Development

Changes should stay within the component boundary they affect. CI is path-aware so CLI, Module/WebUI, and KPM validation can evolve independently while sharing one canonical repository.
