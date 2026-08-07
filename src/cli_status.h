/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _KPATCH_CLI_STATUS_H_
#define _KPATCH_CLI_STATUS_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Stable process exit classes. Kernel errno values are reported on stderr,
 * never returned directly from main where they would be truncated to 8 bits. */
enum kpatch_cli_status {
    KPATCH_CLI_OK = 0,
    KPATCH_CLI_FAILURE = 1,
    KPATCH_CLI_USAGE = 2,
    KPATCH_CLI_PERMISSION = 3,
    KPATCH_CLI_UNSUPPORTED = 4,
    KPATCH_CLI_IO = 5,
};

static inline int cli_status_from_rc(long rc)
{
    if (rc >= 0) return KPATCH_CLI_OK;

    switch ((int)-rc) {
    case EINVAL:
    case ERANGE:
        return KPATCH_CLI_USAGE;
    case EPERM:
    case EACCES:
        return KPATCH_CLI_PERMISSION;
    case ENOSYS:
        return KPATCH_CLI_UNSUPPORTED;
    case EFAULT:
    case EIO:
        return KPATCH_CLI_IO;
    default:
        return KPATCH_CLI_FAILURE;
    }
}

static inline int cli_report_rc(const char *operation, long rc)
{
    if (rc >= 0) return KPATCH_CLI_OK;

    int err = (int)-rc;
    fprintf(stderr, "%s failed: %s (kernel rc=%ld)\n",
            operation ? operation : "operation", strerror(err), rc);
    return cli_status_from_rc(rc);
}

#endif
