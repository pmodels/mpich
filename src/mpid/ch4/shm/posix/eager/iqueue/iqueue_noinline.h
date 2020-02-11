/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2020 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_IQUEUE_NOINLINE_H_INCLUDED
#define POSIX_EAGER_IQUEUE_NOINLINE_H_INCLUDED

#include "iqueue_types.h"
#include "iqueue_impl.h"

int MPIDI_POSIX_iqueue_init(int rank, int size);
int MPIDI_POSIX_iqueue_finalize(void);

#ifdef POSIX_EAGER_INLINE
#define MPIDI_POSIX_eager_init MPIDI_POSIX_iqueue_init
#define MPIDI_POSIX_eager_finalize MPIDI_POSIX_iqueue_finalize
#endif

#endif /* POSIX_EAGER_IQUEUE_NOINLINE_H_INCLUDED */
