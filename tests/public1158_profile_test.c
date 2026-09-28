/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_status.h"
#include "kpatch.h"
#include "supercall.h"

#define PUBLIC1158_SUPERCALL_KPM_EVENT 0x1150L

static int syscall_count;
static const char *last_key;
static long last_event;
static const char *last_source;
static const char *last_args;

long __wrap_syscall(long number, ...)
{
    (void)number;
    errno = 0;
    syscall_count++;

    va_list ap;
    va_start(ap, number);
    last_key = va_arg(ap, const char *);
    long packed = va_arg(ap, long);
    long cmd = packed & 0xffffL;

    if (!last_key || strcmp(last_key, "patchnest-test-key") != 0) {
        va_end(ap);
        errno = EPERM;
        return -1;
    }

    if (cmd == PUBLIC1158_SUPERCALL_KPM_EVENT) {
        last_event = va_arg(ap, long);
        last_source = va_arg(ap, const char *);
        last_args = va_arg(ap, const char *);
        va_end(ap);
        return 0;
    }

    va_end(ap);
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

    char *event_without_key[] = { "POST_FS_DATA" };
    assert(event_main(1, event_without_key) == CLI_EXIT_PERMISSION);
    assert(syscall_count == 0);

    assert(setenv("PATCHNEST_SUPERKEY", "patchnest-test-key", 1) == 0);
    assert(sc_hello() == 0x11581158L);
    assert(syscall_count == 2); /* compact version query + hello */
    assert(last_key && strcmp(last_key, "patchnest-test-key") == 0);

    /* Public1158 native KPM event 0x1150 is explicitly allowed. */
    char *event_args[] = { "POST_FS_DATA", "PatchNest", "boot" };
    int before_event = syscall_count;
    assert(event_main(3, event_args) == CLI_EXIT_OK);
    assert(syscall_count == before_event + 1);
    assert(last_event == 3);
    assert(last_source && strcmp(last_source, "PatchNest") == 0);
    assert(last_args && strcmp(last_args, "boot") == 0);

    char *bad_event[] = { "NOT_A_REAL_EVENT" };
    before_event = syscall_count;
    assert(event_main(1, bad_event) == CLI_EXIT_USAGE);
    assert(syscall_count == before_event);

    /* 0x1100/0x1101 are SU grant/revoke in Public1158: never issue them. */
    int before = syscall_count;
    assert(sc_rehook_syscall(1) == -EOPNOTSUPP);
    assert(sc_rehook_status() == -EOPNOTSUPP);
    assert(syscall_count == before);

    puts("public1158 profile tests: PASS");
    return 0;
}
