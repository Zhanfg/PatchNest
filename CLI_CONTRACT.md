# PatchNest CLI contract

This document defines the process-status and output contract for the PatchNest userspace CLI.

## Exit codes

| Code | Meaning |
| ---: | --- |
| `0` | Command executed successfully. Query values such as `0`/`1` do **not** change this status. |
| `2` | Invalid command or argument. |
| `3` | Permission/authentication failure (`EPERM`, `EACCES`). |
| `4` | Unsupported supercall/ABI (`ENOSYS`, `EOPNOTSUPP`). |
| `5` | Requested object was not found (`ENOENT`). |
| `6` | I/O, userspace-memory, or allocation failure (`EIO`, `EFAULT`, `ENOMEM`). |
| `8` | Other kernel/supercall failure. |

Raw negative errno values must never be returned directly from `main()` or a CLI subcommand. The original errno is written to `stderr` for diagnostics.

## stdout / stderr

- `stdout` is reserved for command results.
- `stderr` is reserved for diagnostics and argument errors.
- `kpm num` prints a non-negative integer only on success.
- `exclude_get <UID>` prints `1` when excluded and `0` when not excluded; both successful states exit with code `0`.
- `exclude_set <UID> <0|1>` prints the verified final state (`0` or `1`) and exits with code `0` only after readback succeeds.

## UID parsing

UID input must contain decimal digits only and must fit exactly in `uid_t`.

Rejected examples include:

- `abc`
- `-1`
- `123junk`
- values outside the `uid_t` range

UID `0` is accepted only as the exact decimal string `0`; malformed input must never silently target UID 0.

## Regression test

Run without GitHub Actions:

```sh
sh tests/run_cli_contract_tests.sh
```

The test wraps the userspace `syscall()` entry point and verifies business-value/exit-code separation, negative errno mapping, strict UID parsing, positive kstorage success returns, and `kpm num` failure handling.
