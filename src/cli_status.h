/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _PATCHNEST_CLI_STATUS_H_
#define _PATCHNEST_CLI_STATUS_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * Stable process exit codes for the PatchNest CLI.
 *
 * Kernel/supercall errno values are diagnostics, not process exit statuses:
 * negative errno values are mapped into the small, documented categories
 * below so callers never depend on shell-specific truncation of negative
 * return values.
 */
enum patchnest_cli_exit {
    CLI_EXIT_OK = 0,
    CLI_EXIT_USAGE = 2,
    CLI_EXIT_PERMISSION = 3,
    CLI_EXIT_UNSUPPORTED = 4,
    CLI_EXIT_NOT_FOUND = 5,
    CLI_EXIT_IO = 6,
    CLI_EXIT_KERNEL = 8,
};

static inline int cli_exit_from_rc(long rc)
{
    if (rc >= 0)
        return CLI_EXIT_OK;

    switch ((int)-rc) {
    case EPERM:
    case EACCES:
        return CLI_EXIT_PERMISSION;
    case ENOSYS:
#ifdef EOPNOTSUPP
    case EOPNOTSUPP:
#endif
        return CLI_EXIT_UNSUPPORTED;
    case ENOENT:
        return CLI_EXIT_NOT_FOUND;
    case EIO:
    case EFAULT:
    case ENOMEM:
        return CLI_EXIT_IO;
    default:
        return CLI_EXIT_KERNEL;
    }
}

static inline int cli_report_rc(const char *operation, long rc)
{
    if (rc >= 0)
        return CLI_EXIT_OK;

    int err = (int)-rc;
    fprintf(stderr, "%s failed: errno=%d (%s)\n",
            operation, err, strerror(err));
    return cli_exit_from_rc(rc);
}

#endif
