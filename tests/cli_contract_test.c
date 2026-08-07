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
static int fail_kstorage_read;
static int fail_kpm_nums;

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
        return fail_kpm_nums ? -EPERM : 7;
    }

    if (cmd == SUPERCALL_KSTORAGE_READ) {
        (void)va_arg(ap, int);
        (void)va_arg(ap, long);
        int *out = va_arg(ap, int *);
        (void)va_arg(ap, long);
        va_end(ap);
        if (fail_kstorage_read)
            return -EPERM;
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

static void test_exclude_query_exit_is_not_business_value(void)
{
    char *argv[] = { "1000" };

    excluded_state = 1;
    fail_kstorage_read = 0;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_OK);

    excluded_state = 0;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_OK);
}

static void test_exclude_query_propagates_kernel_failure(void)
{
    char *argv[] = { "1000" };
    fail_kstorage_read = 1;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_PERMISSION);
    fail_kstorage_read = 0;
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

static void test_kpm_num_preserves_failure(void)
{
    fail_kpm_nums = 1;
    assert(kpm_nums() == CLI_EXIT_PERMISSION);
    fail_kpm_nums = 0;
    assert(kpm_nums() == CLI_EXIT_OK);
}

int main(void)
{
    test_exclude_query_exit_is_not_business_value();
    test_exclude_query_propagates_kernel_failure();
    test_uid_parser_rejects_ambiguous_input();
    test_positive_mutation_return_is_success();
    test_kpm_num_preserves_failure();
    puts("cli contract tests: PASS");
    return 0;
}
