# PatchNest KPM argv v1 (opt-in; no ABI remapping)

The existing `kpatch kpm load FILE "single raw string"`, `ctl0 NAME "raw"`,
and `event EVENT SOURCE "raw"` commands are unchanged. Raw strings remain
opaque; they are **not** parsed as shell tokens.

New commands:
- `kpatch kpm loadv FILE key=value "two words" ""`
- `kpatch kpm ctlv NAME action=sync retries=3`
- `kpatch eventv POST_FS_DATA patchnest reason=boot mode=safe`

The client joins a bounded **argv vector** into the legacy string parameter.
The receiving KPM *must opt in to decoding*. Existing KPMs that interpret a
raw string do not magically gain multi-argument support.

Wire grammar (ASCII headers, byte lengths):
```
PNARGV1:<count>:<byte-length>:<bytes><byte-length>:<bytes>...
```
Example: `PNARGV1:2:3:abc2:xy` means ["abc","xy"].

Limits: maximum 32 arguments, total 1023 bytes plus trailing NUL. Empty
arguments and UTF-8 byte sequences are supported. Lengths are bytes, not
characters. Length-delimited fields are not NUL-terminated. The decoder
returns a slice pointer and length. No shell quoting or interpolation occurs.
Truncated/invalid input is rejected before dispatch, not passed to the kernel.
Callers should not put secrets in command-line parameters (process listings).

Consumers can include `include/patchnest/argv_v1.h`, use
`pn_argv_decode(raw, slices, capacity)`, and distinguish:
- nonnegative: decoded argument count;
- `PN_ARGV_LEGACY`: keep existing raw-string parser;
- `PN_ARGV_INVALID` / `PN_ARGV_NO_SPACE`: fail closed.

Compatibility: all three commands reuse the current supercall interfaces,
and do not modify kernel callback signatures. Event dispatch remains supported
only with a verified PUBLIC1158 kernel ABI and permission; NEXT2026 returns
unsupported rather than guessing a dangerous command ID.

This feature is a userspace/KPM-authoring protocol, **not** an automatic
upgrade of embedded KPMs, and not evidence of full kernel/runtime compatibility.
