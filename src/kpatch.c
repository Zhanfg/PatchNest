/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include "kpatch.h"

#include <errno.h>
#include <stdio.h>

#include "supercall.h"

uint32_t version(void)
{
    return (MAJOR << 16) + (MINOR << 8) + PATCH;
}

long hello(void)
{
    long ret = sc_hello();
    if (ret < 0) return ret;
    if (ret != SUPERCALL_HELLO_MAGIC) return -EPROTO;

    fprintf(stdout, "%s\n", SUPERCALL_HELLO_ECHO);
    return 0;
}

long kpv(void)
{
    long ret = sc_kp_ver();
    if (ret < 0) return ret;

    fprintf(stdout, "%lx\n", ret);
    return 0;
}

long kv(void)
{
    long ret = sc_k_ver();
    if (ret < 0) return ret;

    fprintf(stdout, "%lx\n", ret);
    return 0;
}

long bootlog(void)
{
    return sc_bootlog();
}

long panic(void)
{
    return sc_panic();
}
