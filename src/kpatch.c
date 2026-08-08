/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include "kpatch.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/capability.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <error.h>

#include "cli_status.h"
#include "supercall.h"

uint32_t version()
{
    uint32_t version_code = (MAJOR << 16) + (MINOR << 8) + PATCH;
    return version_code;
}

int hello()
{
    long ret = sc_hello();
    if (ret < 0)
        return cli_report_rc("hello", ret);

    if (ret != (long)sc_expected_hello_magic()) {
        fprintf(stderr,
                "hello failed: incompatible KernelPatch handshake magic 0x%lx (expected 0x%x for %s)\n",
                ret, sc_expected_hello_magic(), sc_abi_name());
        return CLI_EXIT_UNSUPPORTED;
    }

    fprintf(stdout, "%s\n", sc_expected_hello_echo());
    return CLI_EXIT_OK;
}

void kpv()
{
    uint32_t kpv = sc_kp_ver();
    fprintf(stdout, "%x\n", kpv);
}

void kv()
{
    uint32_t kv = sc_k_ver();
    fprintf(stdout, "%x\n", kv);
}

void bootlog()
{
    sc_bootlog();
}

void panic()
{
    sc_panic();
}
