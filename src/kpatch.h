/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#ifndef _KPU_KPATCH_H_
#define _KPU_KPATCH_H_

#include <stdint.h>
#include <unistd.h>
#include "../version"

#ifdef __cplusplus
extern "C"
{
#endif

    uint32_t version(void);

    long hello(void);
    long kpv(void);
    long kv(void);

    long bootlog(void);
    long panic(void);

#ifdef __cplusplus
}
#endif

#endif
