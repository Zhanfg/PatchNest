# PatchNest — KPM Next capability frontier (2026-10-31)

Status: research + passive detection. **Not a flashable, loading, or hooking feature.**

## Grounded reference implementations

- `741afb7/KPatch-Next-EXP` (core @ `0bf6e632`): experimental KPM LKM
  runtime, GKI/fixmap, 6.6+ changes, symbol resolution, BTF; its
  `lkm/README.md` explicitly requires root, target-KMI compilation and
  kernel policy support.
- `inforcqb/susfs4ksu-for-lkm` (release `v2.3.0-r18.6`, 2026-10-30):
  independent SUSFS LKM with per-KMI .ko and `susfs_insmod`; it is **not
  a native KernelPatch KPM**. `/proc/susfs_*` interfaces are only created
  when the relevant hook path is installed.
- `KernelSU-Next/KernelSU-Next` dev: independent LKM-based SUSFS,
  including experimental x86_64 adaptation.
- `Seyud/fshide-kpm`: real KPM using syscall hooks to filter path access,
  and a runtime command interface; this is a narrower function set than
  full SUSFS.
- `meng156sb/kpm-control-demos`: native CTL0, custom supercall and prctl
  control examples. We prefer CTL0 + `PNARGV1` opt-in rather than taking
  ownership of another syscall.

## Design boundaries

| Capability | Native KPM | Standalone LKM | Engineering gate |
| --- | --- | --- | --- |
| Inline/syscall hooks | supported by KernelPatch runtime | possible via kernel APIs/fixmap | PAC/BTI/KCFI, text integrity, ownership |
| KPM init/ctl0/event | native (event ABI must be verified) | separate protocol | strict ABI/version negotiation |
| Filesystem path and dirent filters | seen in fshide KPM | seen in SUSFS LKM | VFS/LSM completeness, race and lifetime tests |
| Native SUSFS control ABI | not established by KPatch-Next | present in SUSFS LKM | do not mistake LKM for KPM |
| Runtime BTF / offsets | EXP exploration | possible in LKM builds | per-KMI struct offsets and symbol provenance |
| Dual runtime coexistence | theoretically possible | theoretically possible | avoid duplicate hook owners and supercall 45 |
| Dynamic install/unload | KPM-specific callbacks | module_init/module_exit | drain RCU, no hooks after free |
| Portability | KernelPatch-compatible ARM64 | strict vermagic/KMI | no version-nearest fallback |

## Architecture direction

1. Keep SukiSU Root Core as the root owner. A KPM broker should not grant
   another root or claim a second supercall channel.
2. Detect existing SUSFS positively before any future LKM action. Absence
   of a visible node **cannot** prove that SUSFS is absent (permissions,
   concealment, hook availability).
3. Route features by ownership: KPM callback extensions vs preexisting
   SUSFS LKM control nodes. Do not stack two implementations of the same
   VFS, LSM, or syscall hook chain without an explicit lease/ownership ABI.
4. Use an explicit backend capability version handshake before enabling
   hook insertion, KPM event dispatch, or any experimental LKM path.
5. No auto-flash, insmod, rmmod, proc writes, or SU-policy edits by
   capability discovery. Require a separate explicit deployment phase.

## Read-only device probe

The installed file `module/tools/kpm_frontier_probe.sh` takes **no
parameters** and works under Android's native `/system/bin/sh`.
Call `sh /data/adb/modules/<module-id>/tools/kpm_frontier_probe.sh`
with the real installed module path.

It emits fixed key=value facts from the live `/proc/sys/kernel/osrelease`,
`/proc/modules`, `/sys/module`, `/proc/susfs_*` and
`/proc/sys/kernel/modules_disabled`, without invoking privileged
syscalls. `kmi=android15-6.6` means only that the release string names a
supported build target, **not** that the binary matches this device's
exact ABI or is safe to load.

For CI fixture testing only, `PN_FRONTIER_FS_ROOT` may refer to a fake
filesystem tree; end users need no options. `not_observed` is not
equivalent to absent. The script always reports manual review needed.

## Further research gates

- Real SukiSU/KernelSU-specific Root Core ownership and syscall 45
  conflicts, including the existing native PatchNest dual-backend PR.
- Verify on OnePlus 13 GKI 6.6: the actual KMI and vendor ABI, SELinux
  policy, symbol availability, BTI/PAC/KCFI and stack protection.
- Compare SUSFS's LSM insertion and unload protection to the KPM hook
  lifecycle; test repeated load/unload and failure rollback in VM first.
- Decide whether direct native KPM SUSFS port is justified after full
  semantics coverage and measurable benefit; never infer it merely from
  a `.ko` loader success.

References:
- https://github.com/inforcqb/susfs4ksu-for-lkm
- https://github.com/inforcqb/susfs4ksu-for-lkm/releases/tag/v2.3.0-r18.6
- https://github.com/741afb7/KPatch-Next-EXP/tree/main/lkm
- https://github.com/KernelSU-Next/KernelSU-Next/tree/dev/kernel/susfs
- https://github.com/Seyud/fshide-kpm
- https://github.com/meng156sb/kpm-control-demos
