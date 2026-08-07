/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2026 rifsxd.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_status.h"
#include "supercall.h"

extern const char *program_name;

_Noreturn static void rehook_usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s rehook help' for more information.\n", program_name);
    else {
        printf("Usage: %s rehook <enable|disable>\n\n", program_name);
        printf(
            "Rehook syscall command.\n\n"
            "help                 Print this help message.\n"
            "enable               Enable rehook syscall.\n"
            "disable              Disable rehook syscall.\n"
            "\n"
            "See also: rehook_status\n"
        );
    }
    exit(status);
}

_Noreturn static void rehook_status_usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s rehook_status help' for more information.\n", program_name);
    else {
        printf("Usage: %s rehook_status\n\n", program_name);
        printf(
            "Check rehook syscall mode status.\n\n"
            "help                 Print this help message.\n"
        );
    }
    exit(status);
}

long set_rehook_mode(int enable)
{
    long rehook_status = sc_rehook_status();
    if (rehook_status < 0)
        return cli_report_rc("rehook status", rehook_status);

    int current_enabled = rehook_status == 1 ? 1 : 0;
    if (current_enabled == enable) {
        fprintf(stdout, "%d\n", current_enabled);
        return CLI_EXIT_OK;
    }

    long rc = sc_rehook_syscall(enable);
    if (rc < 0)
        return cli_report_rc(enable ? "rehook enable" : "rehook disable", rc);

    long verified = sc_rehook_status();
    if (verified < 0)
        return cli_report_rc("rehook verify", verified);
    if ((verified == 1 ? 1 : 0) != enable) {
        fprintf(stderr, "rehook verification failed: requested=%d observed=%ld\n",
                enable, verified);
        return CLI_EXIT_KERNEL;
    }

    fprintf(stdout, "%d\n", enable);
    return CLI_EXIT_OK;
}

long get_rehook_status(void)
{
    long rehook_status = sc_rehook_status();
    if (rehook_status < 0)
        return cli_report_rc("rehook status", rehook_status);

    fprintf(stdout, "%d\n", rehook_status == 1 ? 1 : 0);
    return CLI_EXIT_OK;
}

int kprehook_main(int argc, char **argv)
{
    if (argc != 1)
        rehook_usage(CLI_EXIT_USAGE);

    if (!strcmp(argv[0], "help"))
        rehook_usage(EXIT_SUCCESS);

    if (!strcmp(argv[0], "enable"))
        return (int)set_rehook_mode(1);
    if (!strcmp(argv[0], "disable"))
        return (int)set_rehook_mode(0);

    fprintf(stderr, "Invalid argument: %s\n", argv[0]);
    rehook_usage(CLI_EXIT_USAGE);
}

int kprehook_status_main(int argc, char **argv)
{
    if (argc > 0 && !strcmp(argv[0], "help"))
        rehook_status_usage(EXIT_SUCCESS);
    if (argc != 0)
        rehook_status_usage(CLI_EXIT_USAGE);

    return (int)get_rehook_status();
}
