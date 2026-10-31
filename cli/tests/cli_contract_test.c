/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cli_status.h"
#include "kpatch.h"
#include "kpextension.h"
#include "kpm.h"
#include "../../include/patchnest/argv_v1.h"
#include "uapi/scdefs.h"

const char *program_name = "patchnest-test";

static int excluded_state;
static int kstorage_read_errno;
static int kpm_nums_errno;
static int hello_errno;
static long hello_result = SUPERCALL_HELLO_MAGIC;
static char last_kpm_args[1024];
static int kpm_load_calls;
static int kpm_ctl_calls;

static long command_id(long packed)
{
    return packed & 0xffffL;
}

/*
 * Model libc/Bionic syscall(2), not a raw kernel entry point: kernel -errno is
 * exposed to C as return -1 with errno set to the original error number.
 */
long __wrap_syscall(long number, ...)
{
    (void)number;
    errno = 0;

    va_list ap;
    va_start(ap, number);
    (void)va_arg(ap, void *); /* supercall key */
    long packed_cmd = va_arg(ap, long);
    long cmd = command_id(packed_cmd);

    if (cmd == SUPERCALL_KERNELPATCH_VER) {
        va_end(ap);
        return 0xa05;
    }

    if (cmd == SUPERCALL_HELLO) {
        va_end(ap);
        if (hello_errno) {
            errno = hello_errno;
            return -1;
        }
        return hello_result;
    }

    if (cmd == SUPERCALL_KPM_LOAD) {
        const char *path = va_arg(ap, const char *);
        const char *args = va_arg(ap, const char *);
        (void)va_arg(ap, void *);
        va_end(ap);
        assert(path && strcmp(path, "/data/example.kpm") == 0);
        assert(args && strlen(args) < sizeof(last_kpm_args));
        strcpy(last_kpm_args, args);
        ++kpm_load_calls;
        return 0;
    }

    if (cmd == SUPERCALL_KPM_CONTROL) {
        const char *name = va_arg(ap, const char *);
        const char *args = va_arg(ap, const char *);
        (void)va_arg(ap, char *);
        (void)va_arg(ap, long);
        va_end(ap);
        assert(name && strcmp(name, "demo") == 0);
        assert(args && strlen(args) < sizeof(last_kpm_args));
        strcpy(last_kpm_args, args);
        ++kpm_ctl_calls;
        return 0;
    }

    if (cmd == SUPERCALL_KPM_NUMS) {
        va_end(ap);
        if (kpm_nums_errno) {
            errno = kpm_nums_errno;
            return -1;
        }
        return 7;
    }

    if (cmd == SUPERCALL_KSTORAGE_READ) {
        (void)va_arg(ap, int);
        (void)va_arg(ap, long);
        int *out = va_arg(ap, int *);
        (void)va_arg(ap, long);
        va_end(ap);
        if (kstorage_read_errno) {
            errno = kstorage_read_errno;
            return -1;
        }
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
    errno = ENOSYS;
    return -1;
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

static void test_hello_is_a_real_readiness_gate(void)
{
    hello_errno = 0;
    hello_result = SUPERCALL_HELLO_MAGIC;
    assert(hello() == CLI_EXIT_OK);

    /* A foreign KernelPatch-Public 0x1158 handshake must not report ready. */
    hello_result = 0x11581158L;
    assert(hello() == CLI_EXIT_UNSUPPORTED);

    /* Bionic-style syscall failures preserve their stable error category. */
    hello_errno = EPERM;
    assert(hello() == CLI_EXIT_PERMISSION);

    hello_errno = 0;
    hello_result = SUPERCALL_HELLO_MAGIC;
}

static void test_exclude_query_exit_is_not_business_value(void)
{
    char *argv[] = { "1000" };

    excluded_state = 1;
    kstorage_read_errno = 0;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_OK);

    excluded_state = 0;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_OK);
}

static void test_exclude_query_preserves_bionic_errno(void)
{
    char *argv[] = { "1000" };

    kstorage_read_errno = EPERM;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_PERMISSION);

    kstorage_read_errno = ENOSYS;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_UNSUPPORTED);

    kstorage_read_errno = EFAULT;
    assert(kpexclude_get_main(1, argv) == CLI_EXIT_IO);

    kstorage_read_errno = 0;
}

static void test_uid_parser_rejects_ambiguous_input(void)
{
    char *alpha[] = { "abc" };
    char *negative[] = { "-1" };
    char *suffix[] = { "123junk" };
    char *overflow[] = { "184467440737095516160" };
    char *zero_alias[] = { "00" };

    assert(kpexclude_get_main(1, alpha) == CLI_EXIT_USAGE);
    assert(kpexclude_get_main(1, negative) == CLI_EXIT_USAGE);
    assert(kpexclude_get_main(1, suffix) == CLI_EXIT_USAGE);
    assert(kpexclude_get_main(1, overflow) == CLI_EXIT_USAGE);
    assert(kpexclude_get_main(1, zero_alias) == CLI_EXIT_USAGE);
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
    kstorage_read_errno = EFAULT;
    assert(kpexclude_set_main(2, argv) == CLI_EXIT_IO);
    assert(excluded_state == 0);
    kstorage_read_errno = 0;
}

static void test_kpm_num_preserves_bionic_errno(void)
{
    kpm_nums_errno = EPERM;
    assert(kpm_nums() == CLI_EXIT_PERMISSION);
    kpm_nums_errno = ENOSYS;
    assert(kpm_nums() == CLI_EXIT_UNSUPPORTED);
    kpm_nums_errno = 0;
    assert(kpm_nums() == CLI_EXIT_OK);
}

static void test_vector_arguments_reach_kernel_boundary(void)
{
    char *load[] = { "kpm", "loadv", "/data/example.kpm", "mode=fast", "two words", "" };
    char *control[] = { "kpm", "ctlv", "demo", "action=sync", "retry=2" };
    char *legacy[] = { "kpm", "load", "/data/example.kpm", "raw string stays raw" };
    char *invalid[] = { "kpm", "loadv", "/data/example.kpm", "x" };
    char huge[1024];
    struct pn_argv_slice slices[4] = {{0}};
    memset(huge, 'X', sizeof(huge)-1);
    huge[sizeof(huge)-1] = '\0';
    invalid[3] = huge;
    assert(kpm_main(4, invalid) == CLI_EXIT_USAGE);
    assert(kpm_load_calls == 0);

    assert(kpm_main(6, load) == CLI_EXIT_OK);
    assert(kpm_load_calls == 1);
    assert(pn_argv_decode(last_kpm_args, slices, 4) == 3);
    assert(slices[0].length == 9 && !memcmp(slices[0].data, "mode=fast", 9));
    assert(slices[1].length == 9 && !memcmp(slices[1].data, "two words", 9));
    assert(slices[2].length == 0);

    assert(kpm_main(5, control) == CLI_EXIT_OK);
    assert(kpm_ctl_calls == 1);
    assert(pn_argv_decode(last_kpm_args, slices, 4) == 2);
    assert(slices[0].length == 11 && !memcmp(slices[0].data, "action=sync", 11));

    assert(kpm_main(4, legacy) == CLI_EXIT_OK);
    assert(kpm_load_calls == 2);
    assert(strcmp(last_kpm_args, "raw string stays raw") == 0);

    hello_result = 0x11581158L;
    assert(kpm_main(6, load) == CLI_EXIT_UNSUPPORTED);
    assert(kpm_load_calls == 2);
    hello_result = SUPERCALL_HELLO_MAGIC;
}

int main(void)
{
    test_exit_mapping_is_stable();
    test_hello_is_a_real_readiness_gate();
    test_exclude_query_exit_is_not_business_value();
    test_exclude_query_preserves_bionic_errno();
    test_uid_parser_rejects_ambiguous_input();
    test_positive_mutation_return_is_success();
    test_mutation_aborts_when_pre_read_fails();
    test_kpm_num_preserves_bionic_errno();
    test_vector_arguments_reach_kernel_boundary();
    puts("cli contract tests: PASS");
    return 0;
}
