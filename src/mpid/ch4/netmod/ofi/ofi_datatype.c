/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#ifndef NETMOD_INLINE
#include "ofi_noinline.h"
#endif

int MPIDI_OFI_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_TYPE_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_TYPE_FREE_HOOK);
    return 0;
}

int MPIDI_OFI_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_TYPE_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_TYPE_COMMIT_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_TYPE_COMMIT_HOOK);
    return 0;
}
