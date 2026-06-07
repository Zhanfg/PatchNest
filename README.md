# Kpm-Repo

Official KPatch Module (KPM) catalog for [KPatch-Next](https://github.com/Zhanfg/KPatch-Next-Module).

The KPatch-Next WebUI's **Kpm-Repo** page subscribes to this repository by default and lists the modules declared in `kpm_repo.json`. Users can add additional repositories (forks, third-party catalogs) through the WebUI's "Manage repositories" dialog.

## Layout

- `kpm_repo.json` — the catalog consumed by the WebUI. See schema below.
- `module/kpms/*.c` — the C source for the prebuilt KPM modules listed in the catalog. These are the same sources that are compiled into `.kpm` binaries and shipped as release artifacts in [Zhanfg/KPatch-Next-Module](https://github.com/Zhanfg/KPatch-Next-Module/releases).
- `webroot/index.html` — a static HTML rendering of the catalog for users who prefer to browse modules without the KPatch WebUI.

## Catalog schema

`kpm_repo.json` is a JSON object:

```json
{
  "name": "Repository display name shown in the WebUI",
  "description": "Repository description shown in the WebUI",
  "version": 1,
  "modules": [
    {
      "id": "unique-module-id",
      "name": "Human-readable module name",
      "version": "1.0.0",
      "author": "github-username",
      "description": "What the module does",
      "downloadUrl": "https://github.com/Zhanfg/KPatch-Next-Module/releases/download/vX.Y.Z/module-id-1.0.0.kpm",
      "minKpVersion": "0.13.5",
      "size": 8192,
      "signatureRequired": true,
      "category": "anti-detect",
      "targets": ["Memory", "Mount", "SELinux"],
      "tags": ["stealth", "anti-detect"]
    }
  ]
}
```

### Field reference

| Field | Required | Description |
|-------|----------|-------------|
| `id` | yes | Unique identifier across the catalog. Must match the KPM's `id` field in its own `module.prop`. |
| `name` | yes | Human-readable name shown in the WebUI. |
| `version` | yes | Semver string. The WebUI compares it against the installed module's version to detect updates. |
| `author` | yes | Display name. Typically a GitHub username. |
| `description` | yes | One-paragraph description shown on the module card. |
| `downloadUrl` | yes | Direct HTTPS URL to a `.kpm.zip` (or `.kpm`) file. The WebUI's installer downloads this URL, then runs `install_kpm.sh` on the downloaded archive. **Must be HTTPS** — the URL sanitizer in the WebUI rejects `http://` and other schemes. |
| `minKpVersion` | yes | Minimum KPatch version required. The WebUI displays a warning if the user is below this version. |
| `size` | no | Display-only — the on-disk size of the download in bytes. |
| `signatureRequired` | no | Hint to the WebUI; the actual signing verification is performed by the installer (`install_kpm.sh`) and KPM signature checker (`kpm_verify.sh`). |
| `category` | no | Display-only. The WebUI does not filter by this; it's used by maintainers for grouping. |
| `targets` | no | Display-only — the names of the duck-detector signal cards the module defends against. |
| `tags` | no | Display-only — search-friendly keywords. |

## Adding a new module

1. Drop the KPM source in `module/kpms/<id>.c` (or in a subdirectory).
2. Add an entry to the `modules` array in `kpm_repo.json`.
3. Build the KPM (see [KPatch-Next's CI workflow](https://github.com/Zhanfg/KPatch-Next-Module/blob/main/.github/workflows/build.yaml)) and attach the `.kpm` artifact to a release in KPatch-Next-Module.
4. Set `downloadUrl` in your catalog entry to the release asset URL.
5. Open a PR with the catalog change.

The WebUI pulls this repository on every cold start (and re-pulls on user-initiated refresh from the Kpm-Repo page), so the new module becomes available to all users within one refresh cycle.

## Repository override

Users can add their own repository URLs in the WebUI under **Settings → Manage repositories**, and the maintainer of a custom KPatch-Next build can hard-code a different default URL via `/data/adb/kp-next/repos.json`. See the KPatch-Next documentation for details.

## License

Same license as KPatch-Next (GPLv2 or later, see upstream).
