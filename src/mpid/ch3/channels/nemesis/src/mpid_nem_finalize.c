/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"
#ifndef USE_PMI2_API
#include "pmi.h"
#endif

#include "mpidi_nem_statistics.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_FINALIZE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_FINALIZE);

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
    MPL_free(MPID_nem_mem_region.seg);
    MPL_free(MPID_nem_mem_region.mailboxes.out);
    MPL_free(MPID_nem_mem_region.mailboxes.in);

    MPL_free(MPID_nem_mem_region.local_procs);

#ifdef MEM_REGION_IN_HEAP
    MPL_free(MPID_nem_mem_region_ptr);
#endif /* MEM_REGION_IN_HEAP */

    mpi_errno = MPID_nem_netmod_func->finalize();
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    /* free the shared memory segment */
    mpi_errno = MPIDU_shm_seg_destroy(&MPID_nem_mem_region.memory, MPID_nem_mem_region.num_local);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

#ifdef PAPI_MONITOR
    my_papi_close();
#endif /*PAPI_MONITOR */
    
    if (ENABLE_PVAR_NEM) {
        MPL_free(MPID_nem_fbox_fall_back_to_queue_count);
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

