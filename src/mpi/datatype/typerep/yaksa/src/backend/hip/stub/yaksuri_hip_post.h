/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef YAKSURI_HIP_POST_H_INCLUDED
#define YAKSURI_HIP_POST_H_INCLUDED

static int yaksuri_hip_init_hook(yaksur_gpudriver_hooks_s ** hooks) ATTRIBUTE((unused));
static int yaksuri_hip_init_hook(yaksur_gpudriver_hooks_s ** hooks)
{
    *hooks = NULL;

    return YAKSA_SUCCESS;
}

#endif /* YAKSURI_HIP_POST_H_INCLUDED */
