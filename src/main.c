/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../banner"
#include "uapi/scdefs.h"
#include "cli_status.h"
#include "kpatch.h"
#include "kpm.h"
#include "kpextension.h"
#include "rehook.h"

/* Read-only reference to argv[0]. No fixed-size command buffer is mutated. */
const char *program_name = "kpatch";

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

int main(int argc, char **argv)
{
    if (argv && argv[0] && argv[0][0] != '\0')
        program_name = argv[0];

    if (argc == 1)
        usage(CLI_EXIT_USAGE);

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
        if (strcmp(scmd, cmd_arr[i].scmd))
            continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    if (cmd < 0) {
        fprintf(stderr, "Invalid command: %s\n", scmd);
        return CLI_EXIT_USAGE;
    }

    switch (cmd) {
    case SUPERCALL_HELLO:
        hello();
        return CLI_EXIT_OK;
    case SUPERCALL_KERNELPATCH_VER:
        kpv();
        return CLI_EXIT_OK;
    case SUPERCALL_KERNEL_VER:
        kv();
        return CLI_EXIT_OK;
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
        bootlog();
        return CLI_EXIT_OK;
    case '.':
        panic();
        return CLI_EXIT_OK;
    case 'h':
        usage(EXIT_SUCCESS);
        break;
    case 'v':
        fprintf(stdout, "%x\n", version());
        return CLI_EXIT_OK;
    default:
        return CLI_EXIT_USAGE;
    }

    return CLI_EXIT_OK;
}
