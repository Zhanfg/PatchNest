# PatchNest — Image Patched deployment profile (B)

**Experimental**: branch `feature/patchnest-patched-safe-flex`. This is not a stable installer release and has not yet passed real-device boot/rollback validation.

## Design priority

The patching profile deliberately preserves runtime freedom (**dynamic KPM load, unload, event delivery, static embedding where compatible**) while rejecting unauthenticated or invalid automatic boot loads.

It is **not** an alternative in-tree Kbuild implementation. A's native source is developed separately in `Zhanfg/KernelPatch-Public`.

## Default secure mode on this experimental branch

File: `/data/adb/patchnest/config`

| Setting | Default | Effect |
|---|---|---|
| `KPM_SIGNATURE_POLICY` | `strict` | Only KPMs with a valid detached Ed25519 signature are automatically loaded |
| `KPM_AUTOLOAD_POLICY` | `postboot` | Wait for `sys.boot_completed=1` before loading filesystem KPMs |
| `KPM_AUTOLOAD_POLICY=off` | Optional | Never autoload; use WebUI / CLI to manually load |
| `KPM_AUTOLOAD_POLICY=early` | **Explicit opt-in** | Legacy loading prior to boot completion; higher bootloop risk |
| `KPM_SIGNATURE_POLICY=warn` | **Explicit opt-in** | Permit unsigned modules on automatic load, log a warning |
| `KPM_SIGNATURE_POLICY=off` | **Explicit opt-in** | Disable signature checks (developer use only; high risk) |

Example for local development after boot (not recommended for untrusted modules):

```ini
KPM_SIGNATURE_POLICY=warn
KPM_AUTOLOAD_POLICY=postboot
```

**Important:** signature verification at autoload is a userspace check in `service.sh`. Until the kernel control path also enforces a trusted authorization and verification policy, manual `kpatch kpm load` and other entrypoints may bypass it. Do not treat this branch as a complete kernel trust boundary.

### Signature implementation and portability

The signed message is the raw KPM binary. `module/module/kpm_verify.sh` verifies a 64-byte Ed25519 signature encoded as a 128-character hexadecimal string in a `.kpm.sig` file using:

```text
openssl pkeyutl -verify -rawin -pubin -keyform DER \
  -inkey pub.der -in module.kpm -sigfile signature.bin
```

The 32-byte Ed25519 public key is encoded as an X.509 SubjectPublicKeyInfo DER (12-byte prefix `302a300506032b6570032100`). The previous `openssl dgst -ed25519 -verify` invocation was incorrect and could not provide reliable verification.

If the device's `openssl` binary does not support `pkeyutl -rawin`, verification fails closed. We **must** test Android/ROM variants before shipping a stable ZIP; there is no silent fallback to unverified loading.

### Persistent crash journal and rollback behavior

Before the first automatic KPM load, `service.sh` writes `/data/adb/patchnest/kpm_session_pending`. It is removed only after boot completion and a one-time 30-second observation window. If the device reboots or the service is interrupted during this window, the next service startup sees the marker and refuses all automatic filesystem KPMs.

The marker is deliberately sticky: it is NOT reset just because `kpatch hello` succeeds.

1. After a failed KPM boot, **do not immediately re-enable autoload**. Check `/data/adb/patchnest/service.log`, the last loaded KPM, and boot/kernel logs.
2. Disable or remove the suspect KPM; verify that normal system boot has recovered.
3. Explicitly remove `/data/adb/patchnest/kpm_session_pending` only when ready to resume autoload.

The journal is best effort; it cannot recover a boot that crashes before `/data` is mounted. A broken/incompatible patch to the kernel image requires known-good boot backup and recovery/fastboot access.

### Compatibility and test gates

- Loading `.ko` / `.o` via the freestanding KPM runtime is **not** equivalent to `insmod`; only known compatible objects may be treated as KPM.
- The patched binary must match kernel image/architecture/KMI/CFI constraints; none of these are automatically proven by this shell policy.
- Static embedded KPMs are outside the `service.sh` auto-load policy; enforcing A-grade stability requires separate in-kernel policy.
- `module/module/patch/boot_patch.sh` now verifies a fresh boot-image backup (unpack + SHA256 byte comparison) and validates the repacked image before direct flash. It refuses device flashing when no fresh verified backup exists. This is an intentionally **breaking safety restriction** for repeat patching; offline image export remains possible. Repeated patching needs a reviewed provenance-aware recovery path before release.
- `boot_unpatch.sh` remains a separate recovery component that still needs a matching-slot and hash-enforced restoration audit; do not infer that all recovery risks are solved.
- Embedded KPM candidates must pass bounded ELF format checks and `kptools -l`; missing tools or unsupported objects are rejected instead of being merely warned.
- Device-specific partitions, AVB, slot identity, running root chain and KPM compatibility need independent hardware verification.
- Before any stable build: test the signature verifier on actual Android `openssl`; test strict/warn/early/off; force process termination at each journal stage; test KPM crash + reboot fallback; validate image flash rollback on both slots.

## Current evidence

`module/tests/kpm-signature-smoke.sh` validates a public known-good Ed25519 test vector and four failure cases with `dash` and `bash`. The dedicated CI workflow runs these tests and shell syntax checks. CI passing does **not** imply working kernel patching or Android hardware safety.
