/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

int HYDU_Error_printf_simple(const char *str, ...)
{
    int n;
    va_list list;

    va_start(list, str);
    n = vfprintf(stderr, str, list);
    fflush(stderr);
    va_end(list);

    return n;
}
