/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "mpidig.h"
#include "mpidig_am.h"
#include "mpidch4r.h"

int MPIDIG_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_INIT);

    MPIDI_global.is_ch4u_initialized = 0;
    MPIDIG_am_init();
    MPIDIU_map_create((void **) &(MPIDI_global.win_map), MPL_MEM_RMA);
    MPIDI_global.csel_root = NULL;
    MPIDI_global.is_ch4u_initialized = 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_INIT);
    return mpi_errno;
}

void MPIDIG_finalize(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_FINALIZE);

    MPIDIG_am_finalize();
    MPIDI_global.is_ch4u_initialized = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_FINALIZE);
}
