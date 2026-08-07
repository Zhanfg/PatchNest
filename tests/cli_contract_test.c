/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "cli_status.h"
#include "kpextension.h"
#include "kpm.h"
#include "uapi/scdefs.h"

const char *program_name = "patchnest-test";

static int excluded_state;
static long kstorage_read_error;
static long kpm_nums_error;

static long command_id(long packed)
{
    return packed & 0xffffL;
}

long __wrap_syscall(long number, ...)
{
    (void)number;

    va_list ap;
    va_start(ap, number);
    (void)va_arg(ap, void *); /* supercall key */
    long packed_cmd = va_arg(ap, long);
    long cmd = command_id(packed_cmd);

    if (cmd == SUPERCALL_KERNELPATCH_VER) {
        va_end(ap);
        return 0xa05;
    }

    if (cmd == SUPERCALL_KPM_NUMS) {
        va_end(ap);
        return kpm_nums_error ? kpm_nums_error : 7;
    }

    if (cmd == SUPERCALL_KSTORAGE_READ) {
        (void)va_arg(ap, int);
        (void)va_arg(ap, long);
        int *out = va_arg(ap, int *);
        (void)va_arg(ap, long);
        va_end(ap);
        if (kstorage_read_error)
            return kstorage_read_error;
        *out = excluded_state;
        return (long)sizeof(*out);
    }

    if (cmd == SUPERCALL_KSTORAGE_WRITE) {
        (void)va_arg(ap, int);
        (void)va_arg(ap, long);
        (void)va_arg(ap, void *);
        (void)va_arg(ap, long);
        va_end(ap);
        excluded_state = 1;
        return (long)sizeof(int); /* success may be positive */
    }

    if (cmd == SUPERCALL_KSTORAGE_REMOVE) {
        va_end(ap);
        excluded_state = 0;
        return 0;
    }

    va_end(ap);
    return -ENOSYS;
}

static void test_exit_mapping_is_stable(void)
{
    assert(cli_exit_from_rc(0) == CLI_EXIT_OK);
    assert(cli_exit_from_rc(7) == CLI_EXIT_OK);
    assert(cli_exit_from_rc(-EPERM) == CLI_EXIT_PERMISSION);
    assert(cli_exit_from_rc(-EACCES) == CLI_EXIT_PERMISSION);
    assert(cli_exit_from_rc(-ENOSYS) == CLI_EXIT_UNSUPPORTED);
    assert(cli_exit_from_rc(-ENOENT) == CLI_EXIT_NOT_FOUND);
    assert(cli_exit_from_rc(-EFAULT) == CLI_EXIT_IO);
    assert(cli_exit_from_rc(-EIO) == CLI_EXIT_IO);
    assert(cli_exit_from_rc(-ENOMEM) == CLI_EXIT_IO);
    assert(cli_exit_from_rc(-EINVAL) == CLI_EXIT_KERNEL);
}

static void test_exclude_query_exit_is_not_business_value(void)
{
    char *argv[] = { "1000" };

    excluded_state = 1;
    kstorage_read_error = 0;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_OK);

    excluded_state = 0;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_OK);
}

static void test_exclude_query_propagates_kernel_failures(void)
{
    char *argv[] = { "1000" };

    kstorage_read_error = -EPERM;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_PERMISSION);

    kstorage_read_error = -ENOSYS;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_UNSUPPORTED);

    kstorage_read_error = -EFAULT;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_IO);

    kstorage_read_error = 0;
}

static void test_uid_parser_rejects_ambiguous_input(void)
{
    char *alpha[] = { "abc" };
    char *negative[] = { "-1" };
    char *suffix[] = { "123junk" };
    char *overflow[] = { "184467440737095516160" };

    assert(kpexclude_get_main(1, alpha) == CLI_EXIT_USAGE);
    assert(kpexclude_get_main(1, negative) == CLI_EXIT_USAGE);
    assert(kpexclude_get_main(1, suffix) == CLI_EXIT_USAGE);
    assert(kpexclude_get_main(1, overflow) == CLI_EXIT_USAGE);
}

static void test_positive_mutation_return_is_success(void)
{
    char *argv[] = { "1000", "1" };
    excluded_state = 0;
    assert(kpexclude_set_main(2, argv) == CLI_EXIT_OK);
    assert(excluded_state == 1);
}

static void test_mutation_aborts_when_pre_read_fails(void)
{
    char *argv[] = { "1000", "1" };
    excluded_state = 0;
    kstorage_read_error = -EFAULT;
    assert(kpexclude_set_main(2, argv) == CLI_EXIT_IO);
    assert(excluded_state == 0);
    kstorage_read_error = 0;
}

static void test_kpm_num_preserves_failure(void)
{
    kpm_nums_error = -EPERM;
    assert(kpm_nums() == CLI_EXIT_PERMISSION);
    kpm_nums_error = -ENOSYS;
    assert(kpm_nums() == CLI_EXIT_UNSUPPORTED);
    kpm_nums_error = 0;
    assert(kpm_nums() == CLI_EXIT_OK);
}

int main(void)
{
    test_exit_mapping_is_stable();
    test_exclude_query_exit_is_not_business_value();
    test_exclude_query_propagates_kernel_failures();
    test_uid_parser_rejects_ambiguous_input();
    test_positive_mutation_return_is_success();
    test_mutation_aborts_when_pre_read_fails();
    test_kpm_num_preserves_failure();
    puts("cli contract tests: PASS");
    return 0;
}
