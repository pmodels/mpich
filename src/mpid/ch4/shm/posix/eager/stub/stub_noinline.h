/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_STUB_NOINLINE_H_INCLUDED
#define POSIX_EAGER_STUB_NOINLINE_H_INCLUDED

#include "stub_impl.h"

int MPIDI_POSIX_stub_init(int rank, int size);
int MPIDI_POSIX_stub_finalize(void);

#ifdef POSIX_EAGER_INLINE
#define MPIDI_POSIX_eager_init MPIDI_POSIX_stub_init
#define MPIDI_POSIX_eager_finalize MPIDI_POSIX_stub_finalize
#endif

#endif /* POSIX_EAGER_STUB_NOINLINE_H_INCLUDED */
