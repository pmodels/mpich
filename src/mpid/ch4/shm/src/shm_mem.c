/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

void *MPIDI_SHMI_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *ret;

    ret = MPIDI_POSIX_mpi_alloc_mem(size, info_ptr);

    return ret;
}

int MPIDI_SHMI_mpi_free_mem(void *ptr)
{
    int ret;

    ret = MPIDI_POSIX_mpi_free_mem(ptr);

    return ret;
}
