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

#define PUBLIC1158_SUPERCALL_KPM_EVENT 0x1150L

struct event_name {
    const char *name;
    int value;
};

static int parse_event_name(const char *name)
{
    static const struct event_name events[] = {
        { "PRE_KERNEL_INIT", 1 },
        { "POST_KERNEL_INIT", 2 },
        { "POST_FS_DATA", 3 },
        { "BOOT_COMPLETED", 4 },
        { "MODULE_LOADED", 5 },
        { "MODULE_UNLOADED", 6 },
    };

    if (!name)
        return -1;
    for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); ++i) {
        if (!strcmp(name, events[i].name))
            return events[i].value;
    }
    return -1;
}

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

int event_main(int argc, char **argv)
{
    if (argc < 1 || argc > 3 || !argv) {
        fprintf(stderr, "event requires EVENT [SOURCE] [ARGS]\n");
        return CLI_EXIT_USAGE;
    }

    int event = parse_event_name(argv[0]);
    if (event < 0) {
        fprintf(stderr, "unsupported event name: %s\n", argv[0] ? argv[0] : "(null)");
        return CLI_EXIT_USAGE;
    }

#ifndef PATCHNEST_ABI_PUBLIC1158
    (void)event;
    fprintf(stderr, "event is unsupported by ABI profile %s\n", sc_abi_name());
    return CLI_EXIT_UNSUPPORTED;
#else
    const char *key = sc_key();
    if (!key)
        return cli_report_rc("event", -EACCES);

    const char *source = argc >= 2 ? argv[1] : NULL;
    const char *args = argc >= 3 ? argv[2] : NULL;
    long rc = sc_normalize_syscall_result(
        syscall(__NR_supercall, key, ver_and_cmd(PUBLIC1158_SUPERCALL_KPM_EVENT),
                (long)event, source, args));
    return cli_report_rc("event", rc);
#endif
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
