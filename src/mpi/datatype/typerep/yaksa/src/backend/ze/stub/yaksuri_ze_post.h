/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_ZE_POST_H_INCLUDED
#define YAKSURI_ZE_POST_H_INCLUDED

static int yaksuri_ze_init_hook(yaksur_gpudriver_hooks_s ** hooks) ATTRIBUTE((unused));
static int yaksuri_ze_init_hook(yaksur_gpudriver_hooks_s ** hooks)
{
    *hooks = NULL;

    return YAKSA_SUCCESS;
}

#endif /* YAKSURI_ZE_POST_H_INCLUDED */
