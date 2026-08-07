/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <string.h>

#include "cli_status.h"
#include "kpm.h"
#include "supercall.h"

int kpm_load(const char *path, const char *args)
{
    return (int)sc_kpm_load(path, args, 0);
}

int kpm_control(const char *name, const char *ctl_args)
{
    char buf[4096] = { '\0' };
    int rc = (int)sc_kpm_control(name, ctl_args, buf, sizeof(buf));
    if (rc >= 0 && buf[0]) fprintf(stdout, "%s", buf);
    return rc;
}

int kpm_unload(const char *name)
{
    return (int)sc_kpm_unload(name, 0);
}

int kpm_nums(void)
{
    long nums = sc_kpm_nums();
    if (nums < 0) return (int)nums;
    fprintf(stdout, "%ld\n", nums);
    return 0;
}

int kpm_list(void)
{
    char buf[4096] = {0};
    int rc = (int)sc_kpm_list(buf, sizeof(buf));
    if (rc < 0) return rc;
    if (rc > 0) fprintf(stdout, "%s", buf);
    return 0;
}

int kpm_info(const char *name)
{
    char buf[4096] = {0};
    int rc = (int)sc_kpm_info(name, buf, sizeof(buf));
    if (rc < 0) return rc;
    if (rc > 0) fprintf(stdout, "%s", buf);
    return 0;
}

extern const char program_name[];

static void usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s kpm help' for more information.\n", program_name);
    else {
        printf("Usage: %s kpm <COMMAND> [ARG]...\n\n", program_name);
        fprintf(stdout, ""
                        "KPatch-Next Module command set.\n"
                        "\n"
                        "help                           Print this help message. \n"
                        "load <KPM_PATH> [KPM_ARGS]     Load KPatch-Next Module with KPM_PATH and KPM_ARGS.\n"
                        "ctl0 <KPM_NAME> <CTL_ARGS>     Control KPatch-Next Module named KPM_NAME with CTL_ARGS.\n"
                        "unload <KPM_NAME>              Unload KPatch-Next Module named KPM_NAME.\n"
                        "num                            Get the number of modules that have been loaded.\n"
                        "list                           List names of all loaded modules.\n"
                        "info <KPM_NAME>                Get detailed information about module named KPM_NAME.\n"
                        "");
    }
    exit(status);
}

static int cli_result(const char *operation, int rc)
{
    return rc < 0 ? cli_report_rc(operation, rc) : KPATCH_CLI_OK;
}

int kpm_main(int argc, char **argv)
{
    if (argc < 2) usage(KPATCH_CLI_USAGE);

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
        if (strcmp(scmd, cmd_arr[i].scmd)) continue;
        cmd = cmd_arr[i].cmd;
        break;
    }

    if (cmd < 0) usage(KPATCH_CLI_USAGE);

    const char *path = NULL;
    const char *mod_args = NULL;
    const char *ctl_args = NULL;
    const char *name = NULL;

    switch (cmd) {
    case SUPERCALL_KPM_LOAD:
        if (argc < 3) {
            fprintf(stderr, "module path does not exist\n");
            return KPATCH_CLI_USAGE;
        }
        path = argv[2];
        mod_args = argc < 4 ? NULL : argv[3];
        return cli_result("kpm load", kpm_load(path, mod_args));
    case SUPERCALL_KPM_CONTROL:
        if (argc < 3) {
            fprintf(stderr, "module name does not exist\n");
            return KPATCH_CLI_USAGE;
        }
        if (argc < 4) {
            fprintf(stderr, "control argument does not exist\n");
            return KPATCH_CLI_USAGE;
        }
        name = argv[2];
        ctl_args = argv[3];
        return cli_result("kpm ctl0", kpm_control(name, ctl_args));
    case SUPERCALL_KPM_UNLOAD:
        if (argc < 3) {
            fprintf(stderr, "module name does not exist\n");
            return KPATCH_CLI_USAGE;
        }
        name = argv[2];
        return cli_result("kpm unload", kpm_unload(name));
    case SUPERCALL_KPM_NUMS:
        return cli_result("kpm num", kpm_nums());
    case SUPERCALL_KPM_LIST:
        return cli_result("kpm list", kpm_list());
    case SUPERCALL_KPM_INFO:
        if (argc < 3) {
            fprintf(stderr, "module name does not exist\n");
            return KPATCH_CLI_USAGE;
        }
        name = argv[2];
        return cli_result("kpm info", kpm_info(name));
    case 0:
        usage(EXIT_SUCCESS);
    default:
        usage(KPATCH_CLI_USAGE);
    }

    return KPATCH_CLI_OK;
}
