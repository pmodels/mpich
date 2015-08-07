/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpiu_os_wrappers.h"

static int sense;
static int barrier_init = 0;

#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_barrier_init(MPID_nem_barrier_t *barrier_region, int init_values)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_BARRIER_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_BARRIER_INIT);
    
    MPID_nem_mem_region.barrier = barrier_region;
    if (init_values) {
        OPA_store_int(&MPID_nem_mem_region.barrier->val, 0);
        OPA_store_int(&MPID_nem_mem_region.barrier->wait, 0);
        OPA_write_barrier();
    }
    sense = 0;
    barrier_init = 1;

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_BARRIER_INIT);

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* FIXME: this is not a scalable algorithm because everyone is polling on the same cacheline */
int MPID_nem_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_BARRIER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_BARRIER);

    if (MPID_nem_mem_region.num_local == 1)
        goto fn_exit;

    MPIR_ERR_CHKINTERNAL(!barrier_init, mpi_errno, "barrier not initialized");

    if (OPA_fetch_and_incr_int(&MPID_nem_mem_region.barrier->val) == MPID_nem_mem_region.num_local - 1)
    {
	OPA_store_int(&MPID_nem_mem_region.barrier->val, 0);
	OPA_store_int(&MPID_nem_mem_region.barrier->wait, 1 - sense);
        OPA_write_barrier();
    }
    else
    {
	/* wait */
	while (OPA_load_int(&MPID_nem_mem_region.barrier->wait) == sense)
            MPIU_PW_Sched_yield(); /* skip */
    }
    sense = 1 - sense;

 fn_fail:
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_BARRIER);
    return mpi_errno;
}
