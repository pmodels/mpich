/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_NOINLINE_H_INCLUDED
#define POSIX_EAGER_IQUEUE_NOINLINE_H_INCLUDED

#include "iqueue_types.h"
#include "iqueue_impl.h"

int MPIDI_POSIX_iqueue_init(int rank, int size);
int MPIDI_POSIX_iqueue_post_init(void);
int MPIDI_POSIX_iqueue_set_vcis(MPIR_Comm * comm, int num_vcis);
int MPIDI_POSIX_iqueue_finalize(void);

#ifdef POSIX_EAGER_INLINE
#define MPIDI_POSIX_eager_init MPIDI_POSIX_iqueue_init
#define MPIDI_POSIX_eager_post_init MPIDI_POSIX_iqueue_post_init
#define MPIDI_POSIX_eager_set_vcis MPIDI_POSIX_iqueue_set_vcis
#define MPIDI_POSIX_eager_finalize MPIDI_POSIX_iqueue_finalize
#endif

#endif /* POSIX_EAGER_IQUEUE_NOINLINE_H_INCLUDED */
