/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "kpextension.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_status.h"
#include "supercall.h"

extern const char *program_name;

static void set_usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s exclude_set help' for more information.\n", program_name);
    else {
        printf("Usage: %s exclude_set <UID> <0|1>\n\n", program_name);
        printf(
            "Exclude command.\n\n"
            "help                 Print this help message.\n"
            "<UID> 1              Add UID to exclude list.\n"
            "<UID> 0              Remove UID from exclude list.\n"
            "\n"
            "UID must be unsigned decimal in uid_t range. UID 0 is accepted only as literal `0`.\n"
        );
    }
    exit(status);
}

static void get_usage(int status)
{
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s exclude_get help' for more information.\n", program_name);
    else {
        printf("Usage: %s exclude_get <UID>\n\n", program_name);
        printf(
            "Get exclude command.\n\n"
            "help                 Print this help message.\n"
            "<UID>                Print 1 when excluded, otherwise 0.\n"
        );
    }
    exit(status);
}

static int parse_uid(const char *text, uid_t *uid)
{
    if (!text || !*text || !uid)
        return -EINVAL;

    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p < '0' || *p > '9')
            return -EINVAL;
    }

    errno = 0;
    char *end = NULL;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno == ERANGE || !end || *end != '\0')
        return -ERANGE;

    uid_t converted = (uid_t)value;
    if ((unsigned long long)converted != value)
        return -ERANGE;

    *uid = converted;
    return 0;
}

static int parse_exclude(const char *text, int *exclude)
{
    if (!text || !exclude)
        return -EINVAL;
    if (!strcmp(text, "0")) {
        *exclude = 0;
        return 0;
    }
    if (!strcmp(text, "1")) {
        *exclude = 1;
        return 0;
    }
    return -EINVAL;
}

long set_uid_exclude(uid_t uid, int exclude)
{
    if (exclude != 0 && exclude != 1)
        return -EINVAL;

    long current = sc_get_ap_mod_exclude(uid);
    if (current < 0)
        return current;

    int is_excluded = current ? 1 : 0;
    if (is_excluded == exclude) {
        fprintf(stdout, "%d\n", is_excluded);
        return 0;
    }

    long rc = sc_set_ap_mod_exclude(uid, exclude);
    if (rc < 0)
        return rc;

    long verified = sc_get_ap_mod_exclude(uid);
    if (verified < 0)
        return verified;
    if ((verified ? 1 : 0) != exclude)
        return -EIO;

    fprintf(stdout, "%d\n", exclude);
    return 0;
}

long get_uid_exclude(uid_t uid)
{
    long rc = sc_get_ap_mod_exclude(uid);
    if (rc < 0)
        return rc;

    int value = rc ? 1 : 0;
    fprintf(stdout, "%d\n", value);
    return value;
}

int kpexclude_set_main(int argc, char **argv)
{
    if (argc != 2)
        set_usage(CLI_EXIT_USAGE);

    if (!strcmp(argv[0], "help"))
        set_usage(EXIT_SUCCESS);

    uid_t uid = 0;
    int exclude = 0;

    if (parse_uid(argv[0], &uid) < 0) {
        fprintf(stderr, "invalid UID: %s\n", argv[0]);
        return CLI_EXIT_USAGE;
    }
    if (parse_exclude(argv[1], &exclude) < 0) {
        fprintf(stderr, "exclude must be exactly 0 or 1\n");
        return CLI_EXIT_USAGE;
    }

    long rc = set_uid_exclude(uid, exclude);
    if (rc < 0)
        return cli_report_rc("exclude_set", rc);
    return CLI_EXIT_OK;
}

int kpexclude_get_main(int argc, char **argv)
{
    if (argc != 1)
        get_usage(CLI_EXIT_USAGE);

    if (!strcmp(argv[0], "help"))
        get_usage(EXIT_SUCCESS);

    uid_t uid = 0;
    if (parse_uid(argv[0], &uid) < 0) {
        fprintf(stderr, "invalid UID: %s\n", argv[0]);
        return CLI_EXIT_USAGE;
    }

    long rc = get_uid_exclude(uid);
    if (rc < 0)
        return cli_report_rc("exclude_get", rc);
    return CLI_EXIT_OK;
}
