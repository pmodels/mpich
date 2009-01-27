/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

static int sense;
static int barrier_init = 0;

#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_barrier_init(int num_processes, MPID_nem_barrier_t *barrier_region)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_BARRIER_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_BARRIER_INIT);

    if (num_processes == 1)
        goto fn_exit;

    MPID_nem_mem_region.barrier = barrier_region;
    MPIDU_Atomic_store(&MPID_nem_mem_region.barrier->val, 0);
    MPIDU_Atomic_store(&MPID_nem_mem_region.barrier->wait, 0);
    sense = 0;
    barrier_init = 1;
    MPIDU_Shm_write_barrier();

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_BARRIER_INIT);
 fn_exit:
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* FIXME: this is not a scalable algorithm because everyone is polling on the same cacheline */
int MPID_nem_barrier (int num_processes, int rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_BARRIER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_BARRIER);

    if (num_processes == 1)
        goto fn_exit;
    
    MPIU_ERR_CHKANDJUMP1 (!barrier_init, mpi_errno, MPI_ERR_INTERN, "**intern", "**intern %s", "barrier not initialized");

    if (MPIDU_Atomic_fetch_and_incr(&MPID_nem_mem_region.barrier->val) == MPID_nem_mem_region.num_local - 1)
    {
	MPIDU_Atomic_store(&MPID_nem_mem_region.barrier->val, 0);
	MPIDU_Atomic_store(&MPID_nem_mem_region.barrier->wait, 1 - sense);
        MPIDU_Shm_write_barrier();
    }
    else
    {
	/* wait */
	while (MPIDU_Atomic_load(&MPID_nem_mem_region.barrier->wait) == sense)
            MPIDU_Yield(); /* skip */
    }
    sense = 1 - sense;

 fn_fail:
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_BARRIER);
    return mpi_errno;
}
