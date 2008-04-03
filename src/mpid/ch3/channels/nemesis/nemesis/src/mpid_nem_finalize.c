/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_FINALIZE);

    /* this test is not the right one */
/*     MPIU_Assert(MPID_nem_queue_empty( MPID_nem_mem_region.RecvQ[MPID_nem_mem_region.rank])); */

    mpi_errno = MPID_nem_net_module_finalize();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* free the shared memory segment */
    mpi_errno = MPID_nem_seg_destroy();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

#ifdef PAPI_MONITOR
    my_papi_close();
#endif /*PAPI_MONITOR */

    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ckpt_shutdown()
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_SHUTDOWN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_SHUTDOWN);

    MPID_nem_net_module_ckpt_shutdown();
    munmap (MPID_nem_mem_region.memory.base_addr, MPID_nem_mem_region.memory.max_size);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_SHUTDOWN);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_vc_terminate(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_VC_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_VC_TERMINATE);

    MPIU_Free(((MPIDI_CH3I_VC *)vc->channel_private)->lmt_copy_buf_handle);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_VC_TERMINATE);
    return mpi_errno;
}
