# PatchNest Backend Policy v1 — 2026-11-01

This follows the read-only KPM Frontier v1 probe. No Root Core takeover and
no new supercall/syscall hooks are installed or authorized.

## How the layer works

- Android sh script \`module/tools/kpm_frontier_probe.sh\` emits strict
  \`KPM_FRONTIER_V2\` key/value evidence. It checks all known SUSFS-LKM proc
  *node names*, BTF availability, SELinux enforcing and exact release-tag KMI.
  It does **not** open any SUSFS rules, log path contents or modify the kernel.
- \`webui/backend-policy.js\` validates the complete schema, rejects duplicate
  or unexpected keys, inconsistent signals, unknown fields and version drift.
  The checker is pure and regression-tested with mocked device evidence.
- \`evaluateBackendPolicy()\` reports conservative resource ownership candidates
  for root_core, syscall_dispatch, vfs_lsm and kpm_callback. Every lease is
  \`unverified\`; every mutation is denied. Seeing an LKM in \`/proc/modules\`
  cannot prove hook ownership, and absence cannot prove that another backend
  is not loaded.
- \`webui/backend-status.js\` displays read-only status on the KPM page, with
  a manually clickable refresh. Failed probes are explicitly shown as unknown.

## Denials / arbitration boundaries

| Operation | Gate |
| --- | --- |
| Read-only discovery | allowed |
| Query SUSFS control node contents | **not implemented**; seeing a node is not authentication |
| Load SUSFS-LKM | denied; duplicate evidence or unverifiable KMI/owner |
| Load EXP KernelPatch LKM | denied; duplicate evidence or syscall ownership unknown |
| Register VFS/LSM/syscall hook | denied; no kernel-enforced ownership lease |
| Write SUSFS rules | denied; no verified control ABI/authentication |
| Native KPM ctl0 | denied within this broker; kernel ABI not established by passive evidence |

The policy engine only advises WebUI and *does not intercept other calls* to
the pre-existing KPM loader. It is not a security boundary until a kernel
resident broker with enforced ownership and versioned attestation exists.

## Compatibility safeguards

- Keep SukiSU/U current Root Core unmodified. Never infer a KernelPatch LKM
  is needed or safe merely from release KMI or BTF availability.
- Do not treat v2 \`not_observed\` as absence. Other modules may hide their
  interfaces. No fallthrough to auto-insmod or latest-version guessing.
- Passive probes can be run with zero arguments:
  \`sh /data/adb/modules/PatchNest/tools/kpm_frontier_probe.sh\`
- A module ZIP built from this source includes the probe, broker display and
  tests, but **no SUSFS .ko** and no automatic module load.
- Before active dual backend ownership: handshake with both kernels, document
  exact hook targets and expected originals, verify PAC/BTI/KCFI per target,
  quiesce RCU callers on unload, test crashes and recovery in a VM.

References:
https://github.com/inforcqb/susfs4ksu-for-lkm
https://github.com/741afb7/KPatch-Next-EXP
https://github.com/Seyud/fshide-kpm
