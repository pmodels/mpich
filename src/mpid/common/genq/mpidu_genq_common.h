/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQ_COMMON_H_INCLUDED
#define MPIDU_GENQ_COMMON_H_INCLUDED

#include <stdint.h>

typedef void *(*MPIDU_genq_malloc_fn) (uintptr_t);
typedef void (*MPIDU_genq_free_fn) (void *);

#endif /* ifndef MPIDU_GENQ_COMMON_H_INCLUDED */
