#include "kpextension.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_status.h"
#include "supercall.h"

extern const char program_name[];

static void set_usage(int status)
{
    if (status != EXIT_SUCCESS) {
        fprintf(stderr, "Try `%s exclude_set help' for more information.\n", program_name);
    } else {
        printf("Usage: %s exclude_set [--allow-uid-0] <UID> <0|1>\n\n", program_name);
        printf(
            "Exclude command.\n\n"
            "help                 Print this help message.\n"
            "<UID> 1              Add UID to exclude list.\n"
            "<UID> 0              Remove UID from exclude list.\n"
            "--allow-uid-0        Explicitly permit mutation of UID 0.\n"
        );
    }
    exit(status);
}

static void get_usage(int status)
{
    if (status != EXIT_SUCCESS) {
        fprintf(stderr, "Try `%s exclude_get help' for more information.\n", program_name);
    } else {
        printf("Usage: %s exclude_get <UID>\n\n", program_name);
        printf(
            "Get exclude command.\n\n"
            "help                 Print this help message.\n"
            "<UID>                Check if UID is in exclude list.\n"
        );
    }
    exit(status);
}

static int parse_uid(const char *text, uid_t *uid)
{
    if (!text || !*text || !uid || text[0] == '-') return -EINVAL;

    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0') return -EINVAL;

    uid_t parsed = (uid_t)value;
    if ((unsigned long)parsed != value) return -ERANGE;

    *uid = parsed;
    return 0;
}

static int parse_exclude(const char *text, int *exclude)
{
    if (!text || !exclude) return -EINVAL;
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
    if (exclude != 0 && exclude != 1) return -EINVAL;

    long current = sc_get_ap_mod_exclude(uid);
    if (current < 0) return current;

    int is_excluded = current ? 1 : 0;
    if (is_excluded == exclude) {
        printf("UID %u is already %sin exclude list\n",
               (unsigned int)uid, exclude ? "" : "not ");
        return 0;
    }

    long rc = sc_set_ap_mod_exclude(uid, exclude);
    if (rc < 0) return rc;

    long readback = sc_get_ap_mod_exclude(uid);
    if (readback < 0) return readback;
    if ((readback ? 1 : 0) != exclude) return -EIO;

    printf("UID %u %s exclude list\n",
           (unsigned int)uid, exclude ? "added to" : "removed from");
    return 0;
}

long get_uid_exclude(uid_t uid)
{
    long rc = sc_get_ap_mod_exclude(uid);
    if (rc < 0) return rc;

    printf("UID %u %s in exclude list\n",
           (unsigned int)uid, rc ? "is" : "is not");
    return 0;
}

int kpexclude_set_main(int argc, char **argv)
{
    if (argc == 1 && !strcmp(argv[0], "help")) set_usage(EXIT_SUCCESS);

    bool allow_uid_zero = false;
    if (argc == 3 && !strcmp(argv[0], "--allow-uid-0")) {
        allow_uid_zero = true;
        argc--;
        argv++;
    }
    if (argc != 2) set_usage(KPATCH_CLI_USAGE);

    uid_t uid = 0;
    int exclude = 0;
    int rc = parse_uid(argv[0], &uid);
    if (rc < 0) {
        fprintf(stderr, "Invalid UID: %s\n", argv[0]);
        return KPATCH_CLI_USAGE;
    }
    rc = parse_exclude(argv[1], &exclude);
    if (rc < 0) {
        fprintf(stderr, "exclude must be exactly 0 or 1\n");
        return KPATCH_CLI_USAGE;
    }
    if (uid == 0 && !allow_uid_zero) {
        fprintf(stderr, "Refusing to mutate UID 0 without --allow-uid-0\n");
        return KPATCH_CLI_USAGE;
    }

    long result = set_uid_exclude(uid, exclude);
    return result < 0 ? cli_report_rc("exclude_set", result) : KPATCH_CLI_OK;
}

int kpexclude_get_main(int argc, char **argv)
{
    if (argc == 1 && !strcmp(argv[0], "help")) get_usage(EXIT_SUCCESS);
    if (argc != 1) get_usage(KPATCH_CLI_USAGE);

    uid_t uid = 0;
    int rc = parse_uid(argv[0], &uid);
    if (rc < 0) {
        fprintf(stderr, "Invalid UID: %s\n", argv[0]);
        return KPATCH_CLI_USAGE;
    }

    long result = get_uid_exclude(uid);
    return result < 0 ? cli_report_rc("exclude_get", result) : KPATCH_CLI_OK;
}
