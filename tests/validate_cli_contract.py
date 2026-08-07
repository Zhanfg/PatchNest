#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def main() -> int:
    main_c = read("src/main.c")
    ext_c = read("src/kpextension.c")
    sc_h = read("src/supercall.h")
    kpm_c = read("src/kpm.c")
    rehook_c = read("src/rehook.c")
    status_h = read("src/cli_status.h")

    require("strcat(" not in main_c, "main.c still uses unbounded strcat")
    require("snprintf(program_name, sizeof(program_name)" in main_c,
            "program name is not bounded")
    require("strtoul(" in ext_c and "atoi(" not in ext_c,
            "exclude UID/value parsing is not strict")
    require("--allow-uid-0" in ext_c,
            "UID 0 mutation lacks explicit opt-in")
    require("long readback = sc_get_ap_mod_exclude(uid);" in ext_c,
            "exclude mutation lacks readback")
    require("if (rc == -ENOENT) return 0;" in sc_h,
            "missing-record semantics are not explicit")
    require(re.search(r"if \(rc < 0\) return rc;\s*return exclude \? 1 : 0;", sc_h),
            "exclude query still swallows kernel errors")
    require("if (nums < 0) return (int)nums;" in kpm_c,
            "kpm num still prints kernel errors as data")
    require("cli_report_rc" in kpm_c and "cli_report_rc" in rehook_c,
            "CLI commands do not normalize negative kernel results")
    require("KPATCH_CLI_PERMISSION = 3" in status_h and
            "KPATCH_CLI_UNSUPPORTED = 4" in status_h and
            "KPATCH_CLI_IO = 5" in status_h,
            "stable CLI error classes are missing")
    require("return command_result(\"hello\", hello());" in main_c and
            "return command_result(\"kpver\", kpv());" in main_c,
            "top-level supercall failures are still ignored")

    print("CLI safety and exit-contract source checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
