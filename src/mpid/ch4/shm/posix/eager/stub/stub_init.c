/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_STUB_INIT_H_INCLUDED
#define POSIX_EAGER_STUB_INIT_H_INCLUDED

#include "stub_noinline.h"

int MPIDI_POSIX_stub_init(int rank, int size)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_stub_finalize()
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* POSIX_EAGER_STUB_INIT_H_INCLUDED */
