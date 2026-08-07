/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../banner"
#include "cli_status.h"
#include "uapi/scdefs.h"
#include "kpatch.h"
#include "kpm.h"
#include "kpextension.h"
#include "rehook.h"

char program_name[128] = "kpatch";

static void initialize_program_name(const char *argv0)
{
    if (!argv0 || !*argv0) return;

    const char *name = strrchr(argv0, '/');
    name = (name && name[1]) ? name + 1 : argv0;

    /* Deterministic truncation is safe for display-only text. Never append to
     * this fixed buffer, so an attacker-controlled argv[0] cannot overflow it. */
    (void)snprintf(program_name, sizeof(program_name), "%s", name);
}

static void usage(int status)
{
    if (status != EXIT_SUCCESS) {
        fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
    } else {
        fprintf(stdout, "\nKPatch-Next userspace cli.\n");
        fprintf(stdout, KERNEL_PATCH_BANNER);
        fprintf(stdout,
                " \n"
                "Options: \n"
                "%s -h, --help       Print this help message. \n"
                "%s -v, --version    Print version. \n"
                "\n",
                program_name, program_name);
        fprintf(stdout, "Usage: %s <COMMAND> [-h, --help] [COMMAND_ARGS]...\n", program_name);
        fprintf(stdout,
                "\n"
                "Commands:\n"
                "hello              If KPatch-Next installed, '%s' will be echoed.\n"
                "kpver              Print KPatch-Next version.\n"
                "kver               Print Kernel version.\n"
                "kpm                KPatch-Next Module manager.\n"
                "exclude_set        Manage the exclude list.\n"
                "exclude_get        Get exclude list status.\n"
                "rehook             Set rehook mode (0=off, 1=target, 2=minimal).\n"
                "rehook_status      Check current rehook mode.\n"
                "\n",
                SUPERCALL_HELLO_ECHO);
    }
    exit(status);
}

static int command_result(const char *operation, long rc)
{
    return rc < 0 ? cli_report_rc(operation, rc) : KPATCH_CLI_OK;
}

int main(int argc, char **argv)
{
    initialize_program_name(argc > 0 ? argv[0] : NULL);

    if (argc == 1) usage(KPATCH_CLI_USAGE);

    const char *scmd = argv[1];
    int cmd = -1;

    struct
    {
        const char *scmd;
        int cmd;
    } cmd_arr[] = {
        { "hello", SUPERCALL_HELLO },
        { "kpver", SUPERCALL_KERNELPATCH_VER },
        { "kver", SUPERCALL_KERNEL_VER },
        { "", 'K' },
        { "kpm", 'k' },
        { "exclude_set", 'e' },
        { "exclude_get", 'g' },
        { "rehook", 'r' },
        { "rehook_status", 'q' },
        { "bootlog", 'l' },
        { "panic", '.' },
        { "--help", 'h' },
        { "-h", 'h' },
        { "--version", 'v' },
        { "-v", 'v' },
    };

    for (size_t i = 0; i < sizeof(cmd_arr) / sizeof(cmd_arr[0]); i++) {
        if (strcmp(scmd, cmd_arr[i].scmd)) continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    if (cmd < 0) {
        fprintf(stderr, "Invalid command: %s\n", scmd);
        return KPATCH_CLI_USAGE;
    }

    switch (cmd) {
    case SUPERCALL_HELLO:
        return command_result("hello", hello());
    case SUPERCALL_KERNELPATCH_VER:
        return command_result("kpver", kpv());
    case SUPERCALL_KERNEL_VER:
        return command_result("kver", kv());
    case 'k':
        return kpm_main(argc - 1, argv + 1);
    case 'e':
        return kpexclude_set_main(argc - 2, argv + 2);
    case 'g':
        return kpexclude_get_main(argc - 2, argv + 2);
    case 'r':
        return kprehook_main(argc - 2, argv + 2);
    case 'q':
        return kprehook_status_main(argc - 2, argv + 2);
    case 'l':
        return command_result("bootlog", bootlog());
    case '.':
        return command_result("panic", panic());
    case 'h':
        usage(EXIT_SUCCESS);
        break;
    case 'v':
        fprintf(stdout, "%x\n", version());
        break;
    default:
        return KPATCH_CLI_USAGE;
    }

    return KPATCH_CLI_OK;
}
