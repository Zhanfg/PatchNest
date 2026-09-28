# Versioning and release namespaces

PatchNest components share one canonical repository but do not use lockstep versions.

| Component | Current baseline | Compatibility source |
|---|---|---|
| CLI | `0.13.5-2` plus post-release hardening | `cli/SOURCE_IMPORT_MANIFEST.json` and `cli/SOURCE_DIVERGENCE_MANIFEST.json` |
| Module/WebUI | `0.4.1-rc2` | `module/version.properties`, `module/module/module.prop`, `module/update.json` |
| KPM catalog | schema/catalog version `1` | `kpms/kpm_repo.json` and each entry's compatibility fields |

Future canonical tags use component namespaces when a release is published:

- CLI: `cli-v<version>`
- Module/WebUI: `module-v<version>`
- KPM catalog/artifacts: `kpm-v<version>`

Historical tags and releases are not renamed. The legacy CLI tag `0.13.5-2` and standalone Module `v*` releases remain immutable provenance.

Compatibility is explicit rather than inferred from tag numbers. Module dependencies stay pinned by `module/version.properties`; installable KPM entries declare `minKpVersion`, `minPatchNestVersion`, tested kernel ranges, and artifact digests.
