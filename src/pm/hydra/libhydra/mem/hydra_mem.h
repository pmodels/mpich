/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_MEM_H_INCLUDED
#define HYDRA_MEM_H_INCLUDED

#include "hydra_base.h"
#include "hydra_err.h"

#define HYD_MALLOC(p, type, size, status)                       \
    do {                                                        \
        (p) = NULL; /* initialize p in case assert fails */     \
        HYD_ASSERT(size, status);                               \
        (p) = (type) MPL_malloc((size));                        \
        if ((p) == NULL)                                        \
            HYD_ERR_SETANDJUMP((status), HYD_ERR_OUT_OF_MEMORY, \
                               "failed to allocate %d bytes\n", \
                               (int) (size));                   \
    } while (0)

#define HYD_REALLOC(p, type, size, status)                      \
    do {                                                        \
        HYD_ASSERT(size, status);                               \
        (p) = (type) MPL_realloc((p),(size));                   \
        if ((p) == NULL)                                        \
            HYD_ERR_SETANDJUMP((status), HYD_ERR_OUT_OF_MEMORY, \
                               "failed to allocate %d bytes\n", \
                               (int) (size));                   \
    } while (0)

#endif /* HYDRA_MEM_H_INCLUDED */
