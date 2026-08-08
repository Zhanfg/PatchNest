/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "supercall.h"

static int syscall_count;
static const char *last_key;

long __wrap_syscall(long number, ...)
{
    (void)number;
    errno = 0;
    syscall_count++;

    va_list ap;
    va_start(ap, number);
    last_key = va_arg(ap, const char *);
    long packed = va_arg(ap, long);
    va_end(ap);

    long cmd = packed & 0xffffL;
    if (!last_key || strcmp(last_key, "patchnest-test-key") != 0) {
        errno = EPERM;
        return -1;
    }

    if (cmd == SUPERCALL_KERNELPATCH_VER)
        return 0xa05;
    if (cmd == SUPERCALL_HELLO)
        return 0x11581158L;

    errno = ENOSYS;
    return -1;
}

int main(void)
{
    unsetenv("PATCHNEST_SUPERKEY");
    syscall_count = 0;
    assert(strcmp(sc_abi_name(), "public1158") == 0);
    assert(sc_expected_hello_magic() == 0x11581158u);
    assert(strcmp(sc_expected_hello_echo(), "hello1158") == 0);
    assert(!sc_rehook_supported());

    /* Missing key fails before the kernel ABI is touched. */
    assert(sc_hello() == -EACCES);
    assert(syscall_count == 0);

    assert(setenv("PATCHNEST_SUPERKEY", "patchnest-test-key", 1) == 0);
    assert(sc_hello() == 0x11581158L);
    assert(syscall_count == 2); /* compact version query + hello */
    assert(last_key && strcmp(last_key, "patchnest-test-key") == 0);

    /* 0x1100/0x1101 are SU grant/revoke in Public1158: never issue them. */
    int before = syscall_count;
    assert(sc_rehook_syscall(1) == -EOPNOTSUPP);
    assert(sc_rehook_status() == -EOPNOTSUPP);
    assert(syscall_count == before);

    puts("public1158 profile tests: PASS");
    return 0;
}
