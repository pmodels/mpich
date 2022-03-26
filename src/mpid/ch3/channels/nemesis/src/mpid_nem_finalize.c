/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"

#include "mpidi_nem_statistics.h"
#include "mpidu_init_shm.h"
#include "mpidi_ch3_impl.h"

int MPID_nem_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_CH3_SHM_Finalize();
    MPIR_ERR_CHECK(mpi_errno);

    /* this test is not the right one */
/*     MPIR_Assert(MPID_nem_queue_empty( MPID_nem_mem_region.RecvQ[MPID_nem_mem_region.rank])); */

    /* these are allocated in MPID_nem_mpich_init, not MPID_nem_init */
    MPL_free(MPID_nem_recv_seqno);
    MPL_free(MPID_nem_fboxq_elem_list);

    /* from MPID_nem_init */
    MPL_free(MPID_nem_mem_region.FreeQ);
    MPL_free(MPID_nem_mem_region.RecvQ);
    MPL_free(MPID_nem_mem_region.local_ranks);
    if (MPID_nem_mem_region.ext_procs > 0)
        MPL_free(MPID_nem_mem_region.ext_ranks);
    MPL_free(MPID_nem_mem_region.mailboxes.out);
    MPL_free(MPID_nem_mem_region.mailboxes.in);

    MPL_free(MPID_nem_mem_region.local_procs);

#ifdef MEM_REGION_IN_HEAP
    MPL_free(MPID_nem_mem_region_ptr);
#endif /* MEM_REGION_IN_HEAP */

    mpi_errno = MPID_nem_netmod_func->finalize();
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    /* free the shared memory segment */
    mpi_errno = MPIDU_Init_shm_free(MPID_nem_mem_region.shm_ptr);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

#ifdef PAPI_MONITOR
    my_papi_close();
#endif /*PAPI_MONITOR */
    
    if (ENABLE_PVAR_NEM) {
        MPL_free(MPID_nem_fbox_fall_back_to_queue_count);
    }

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

