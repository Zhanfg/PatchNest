/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli_status.h"
#include "kpm.h"
#include "supercall.h"

int kpm_load(const char *path, const char *args)
{
    long rc = sc_kpm_load(path, args, 0);
    if (rc < 0)
        return cli_report_rc("kpm load", rc);
    return CLI_EXIT_OK;
}

int kpm_control(const char *name, const char *ctl_args)
{
    char buf[4096] = { '\0' };
    long rc = sc_kpm_control(name, ctl_args, buf, sizeof(buf));
    if (rc < 0)
        return cli_report_rc("kpm control", rc);

    if (buf[0] != '\0')
        fprintf(stdout, "%s", buf);
    return CLI_EXIT_OK;
}

int kpm_unload(const char *name)
{
    long rc = sc_kpm_unload(name, 0);
    if (rc < 0)
        return cli_report_rc("kpm unload", rc);
    return CLI_EXIT_OK;
}

int kpm_nums(void)
{
    long nums = sc_kpm_nums();
    if (nums < 0)
        return cli_report_rc("kpm num", nums);

    fprintf(stdout, "%ld\n", nums);
    return CLI_EXIT_OK;
}

int kpm_list(void)
{
    char buf[4096] = {0};
    long rc = sc_kpm_list(buf, sizeof(buf));
    if (rc < 0)
        return cli_report_rc("kpm list", rc);

    if (rc > 0)
        fprintf(stdout, "%s", buf);
    return CLI_EXIT_OK;
}

int kpm_info(const char *name)
{
    char buf[4096] = {0};
    long rc = sc_kpm_info(name, buf, sizeof(buf));
    if (rc < 0)
        return cli_report_rc("kpm info", rc);

    if (rc > 0)
        fprintf(stdout, "%s", buf);
    return CLI_EXIT_OK;
}

extern const char *program_name;

static void usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s kpm help' for more information.\n", program_name);
    else {
        printf("Usage: %s kpm <COMMAND> [ARG]...\n\n", program_name);
        fprintf(stdout,
                "KPatch-Next Module command set.\n"
                "\n"
                "help                           Print this help message. \n"
                "load <KPM_PATH> [KPM_ARGS]     Load KPatch-Next Module with KPM_PATH and KPM_ARGS.\n"
                "ctl0 <KPM_NAME> <CTL_ARGS>     Control KPatch-Next Module named KPM_NAME with CTL_ARGS.\n"
                "unload <KPM_NAME>              Unload KPatch-Next Module named KPM_NAME.\n"
                "num                            Get the number of modules that have been loaded.\n"
                "list                           List names of all loaded modules.\n"
                "info <KPM_NAME>                Get detailed information about module named KPM_NAME.\n");
    }
    exit(status);
}

int kpm_main(int argc, char **argv)
{
    if (argc < 2)
        usage(CLI_EXIT_USAGE);

    const char *scmd = argv[1];
    int cmd = -1;

    struct
    {
        const char *scmd;
        int cmd;
    } cmd_arr[] = {
        { "load", SUPERCALL_KPM_LOAD },
        { "ctl0", SUPERCALL_KPM_CONTROL },
        { "unload", SUPERCALL_KPM_UNLOAD },
        { "num", SUPERCALL_KPM_NUMS },
        { "list", SUPERCALL_KPM_LIST },
        { "info", SUPERCALL_KPM_INFO },
        { "help", 0 },
    };

    for (size_t i = 0; i < sizeof(cmd_arr) / sizeof(cmd_arr[0]); i++) {
        if (strcmp(scmd, cmd_arr[i].scmd))
            continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    if (cmd < 0)
        usage(CLI_EXIT_USAGE);

    switch (cmd) {
    case SUPERCALL_KPM_LOAD:
        if (argc < 3) {
            fprintf(stderr, "module path does not exist\n");
            return CLI_EXIT_USAGE;
        }
        return kpm_load(argv[2], argc < 4 ? NULL : argv[3]);
    case SUPERCALL_KPM_CONTROL:
        if (argc < 3) {
            fprintf(stderr, "module name does not exist\n");
            return CLI_EXIT_USAGE;
        }
        if (argc < 4) {
            fprintf(stderr, "control argument does not exist\n");
            return CLI_EXIT_USAGE;
        }
        return kpm_control(argv[2], argv[3]);
    case SUPERCALL_KPM_UNLOAD:
        if (argc < 3) {
            fprintf(stderr, "module name does not exist\n");
            return CLI_EXIT_USAGE;
        }
        return kpm_unload(argv[2]);
    case SUPERCALL_KPM_NUMS:
        return kpm_nums();
    case SUPERCALL_KPM_LIST:
        return kpm_list();
    case SUPERCALL_KPM_INFO:
        if (argc < 3) {
            fprintf(stderr, "module name does not exist\n");
            return CLI_EXIT_USAGE;
        }
        return kpm_info(argv[2]);
    case 0:
        usage(EXIT_SUCCESS);
        break;
    default:
        usage(CLI_EXIT_USAGE);
    }

    return CLI_EXIT_OK;
}
