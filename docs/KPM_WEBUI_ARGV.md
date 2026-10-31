# PatchNest WebUI KPM multi-argument UI

Control and load dialogs support optional PNARGV1 multi-argument mode:
one argument per line, with spaces/quotes preserved literally.
Default unchecked, so all legacy KPMs keep receiving the original raw
argument string. This UI requires the CLI `loadv` and `ctlv` merged
in Zhanfg/PatchNest#14.

When using multi-argv **load**, the WebUI forces temporary loading:
persistent service.sh boot replay is not parameter-aware yet. This avoids
silent loss of custom configuration after reboot. The KPM must opt in
to decoding with `patchnest_argv_v1.h`.

A frontend byte/count validator mirrors the CLI's 1023-byte payload
and 32-argument bounds; CLI/kernel validation remains authoritative.
No secrets should be passed in these arguments (process argv may be visible).

## Legacy boot replay compatibility

Legacy `.args` files remain a single raw argument. The module's boot
service and ZIP installer now invoke `kpatch kpm load FILE "RAW"` without
inserting `--` or shell-splitting values. The shell is not asked to parse
the raw text as command syntax. Logs report only whether args are present,
not their contents.

This does **not** enable `loadv` persistence across boots; multi-argv
mode continues to use temporary loading until a versioned persistence
format and recovery policy are specified.
