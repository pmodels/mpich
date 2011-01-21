/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

#define NULL_CONTEXT_ID -1

static int barrier (MPID_Comm *comm_ptr, int *errflag);
static int alloc_barrier_vars (MPID_Comm *comm, MPID_nem_barrier_vars_t **vars);

static MPID_Collops collective_functions = {
    0,    /* ref_count */
    barrier, /* Barrier */
    NULL, /* Bcast */
    NULL, /* Gather */
    NULL, /* Gatherv */
    NULL, /* Scatter */
    NULL, /* Scatterv */
    NULL, /* Allgather */
    NULL, /* Allgatherv */
    NULL, /* Alltoall */
    NULL, /* Alltoallv */
    NULL, /* Alltoallw */
    NULL, /* Reduce */
    NULL, /* Allreduce */
    NULL, /* Reduce_scatter */
    NULL, /* Scan */
    NULL  /* Exscan */
};

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_comm_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_comm_create (MPID_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMM_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_COMM_CREATE);
    comm->ch.barrier_vars = NULL;

    mpi_errno = MPIU_Find_local_and_external(comm, &comm->ch.local_size, &comm->ch.local_rank,
					     &comm->ch.local_ranks, &comm->ch.external_size,
					     &comm->ch.external_rank, &comm->ch.external_ranks,
                                             &comm->ch.intranode_table, &comm->ch.internode_table);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    comm->coll_fns = &collective_functions;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_COMM_CREATE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_comm_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_comm_destroy (MPID_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);
    if (comm->ch.barrier_vars && OPA_fetch_and_decr_int(&comm->ch.barrier_vars->usage_cnt) == 1)
    {
        OPA_write_barrier();
        OPA_store_int(&comm->ch.barrier_vars->context_id, NULL_CONTEXT_ID);
    }
    if (comm->ch.local_size)
        MPIU_Free (comm->ch.local_ranks);
    if (comm->ch.external_size)
        MPIU_Free (comm->ch.external_ranks);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME alloc_barrier_vars
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int alloc_barrier_vars (MPID_Comm *comm, MPID_nem_barrier_vars_t **vars)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int c;

    for (i = 0; i < MPID_NEM_NUM_BARRIER_VARS; ++i)
    {
	c = OPA_cas_int(&MPID_nem_mem_region.barrier_vars[i].context_id, NULL_CONTEXT_ID, comm->context_id);
        if (c == NULL_CONTEXT_ID || c == comm->context_id)
        {
            *vars = &MPID_nem_mem_region.barrier_vars[i];
	    OPA_write_barrier();
            OPA_incr_int(&(*vars)->usage_cnt);
            goto fn_exit;
        }
    }

    *vars = NULL;

 fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME barrier
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int msg_barrier (MPID_Comm *comm_ptr, int rank, int size, int *rank_array)
{
    int mpi_errno = MPI_SUCCESS;
    int src, dst, mask;
    MPI_Comm comm = comm_ptr->handle;

    mask = 0x1;
    while (mask < size)
    {
        dst = rank_array[(rank + mask) % size];
        src = rank_array[(rank - mask + size) % size];
        mpi_errno = MPIC_Sendrecv (NULL, 0, MPI_BYTE, dst, MPIR_BARRIER_TAG,
                                   NULL, 0, MPI_BYTE, src, MPIR_BARRIER_TAG, comm, MPI_STATUS_IGNORE);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        mask <<= 1;
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME barrier
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int barrier (MPID_Comm *comm_ptr, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_barrier_vars_t *barrier_vars;
    int local_size = comm_ptr->ch.local_size;
    int external_size = comm_ptr->ch.external_size;

    /* Trivial barriers return immediately */
    if (comm_ptr->local_size == 1)
        return MPI_SUCCESS;

    /* Only one collective operation per communicator can be active at any
       time */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER (comm_ptr);

    if (local_size == 1)
    {
        /* there are only external processes -- do msg barrier only */
        mpi_errno = msg_barrier (comm_ptr, comm_ptr->ch.external_rank, external_size, comm_ptr->ch.external_ranks);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        goto fn_exit;
    }

    if (comm_ptr->ch.barrier_vars == NULL)
    {
        mpi_errno = alloc_barrier_vars (comm_ptr, &comm_ptr->ch.barrier_vars);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        if (comm_ptr->ch.barrier_vars == NULL)
        {
            /* no barrier_vars left -- revert to safe but inefficient
               implementation: do a barrier using messages with local
               procs, then with external procs, then again with local
               procs. */
            /* FIXME: need a better solution here.  e.g., allocate
               some barrier_vars on the first barrier for the life of
               the communicator (as is the case now), others must be
               allocated for each barrier, then released.  If we run
               out of barrier_vars after that, then use msg_barrier.
            */
            mpi_errno = msg_barrier (comm_ptr, comm_ptr->ch.local_rank, local_size, comm_ptr->ch.local_ranks);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);

            if (comm_ptr->ch.local_rank == 0)
            {
                mpi_errno = msg_barrier (comm_ptr, comm_ptr->ch.external_rank, external_size, comm_ptr->ch.external_ranks);
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            }

            mpi_errno = msg_barrier (comm_ptr, comm_ptr->ch.local_rank, local_size, comm_ptr->ch.local_ranks);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);

            goto fn_exit;
        }
    }

    barrier_vars = comm_ptr->ch.barrier_vars;

    if (external_size == 1)
    {
        /* there are only local procs -- do shared memory barrier only */
        int prev;
        int sense;

        sense = OPA_load_int(&barrier_vars->sig);
        OPA_read_barrier();

        prev = OPA_fetch_and_incr_int(&barrier_vars->cnt);
        if (prev == local_size - 1)
        {
            OPA_store_int(&barrier_vars->cnt, 0);
            OPA_write_barrier();
            OPA_store_int(&barrier_vars->sig, 1 - sense);
        }
        else
        {
            while (OPA_load_int(&barrier_vars->sig) == sense)
                MPIU_PW_Sched_yield();
        }

        goto fn_exit;
    }

    /* there are both local and external processes */

    if (comm_ptr->ch.local_rank == 0)
    {
        /* do barrier between local and external */
        int external_rank = comm_ptr->ch.external_rank;
        int *external_ranks = comm_ptr->ch.external_ranks;

        /* wait for local procs to reach barrier */
        if (local_size > 1)
            while (OPA_load_int(&barrier_vars->sig0) == 0)
                MPIU_PW_Sched_yield();

        /* now do a barrier with external processes */
        mpi_errno = msg_barrier (comm_ptr, external_rank, external_size, external_ranks);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        /* reset ctr and release local procs */
        if (local_size > 1)
        {
            OPA_store_int(&barrier_vars->sig0, 0);
            OPA_store_int(&barrier_vars->cnt, 0);
	    OPA_write_barrier();
	    OPA_store_int(&barrier_vars->sig, 1 - OPA_load_int(&barrier_vars->sig));
        }
    }
    else
    {
        /* just do the local barrier -- Decrement a counter.  If
           counter is 1 (i.e., only root is left), set sig0 to signal
           root.  Then, wait on signal variable. */
        int prev;
        int sense;
        sense = OPA_load_int(&barrier_vars->sig);
        OPA_read_barrier();

        prev = OPA_fetch_and_incr_int(&barrier_vars->cnt);
        if (prev == local_size - 2)  /* - 2 because it's the value before we added 1 and we're not waiting for root */
	{
	    OPA_write_barrier();
	    OPA_store_int(&barrier_vars->sig0, 1);
	}

        while (OPA_load_int(&barrier_vars->sig) == sense)
            MPIU_PW_Sched_yield();
    }

 fn_exit:
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier_vars_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_barrier_vars_init (MPID_nem_barrier_vars_t *barrier_region)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_BARRIER_VARS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_BARRIER_VARS_INIT);
    if (MPID_nem_mem_region.local_rank == 0)
        for (i = 0; i < MPID_NEM_NUM_BARRIER_VARS; ++i)
        {
            OPA_store_int(&barrier_region[i].context_id, NULL_CONTEXT_ID);
            OPA_store_int(&barrier_region[i].usage_cnt, 0);
            OPA_store_int(&barrier_region[i].cnt, 0);
            OPA_store_int(&barrier_region[i].sig0, 0);
            OPA_store_int(&barrier_region[i].sig, 0);
        }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_BARRIER_VARS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier_vars_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_coll_barrier_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_COLL_BARRIER_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_COLL_BARRIER_INIT);

/*     mpi_errno = MPIDI_CH3I_comm_create (MPIR_Process.comm_world); */
/*     if (mpi_errno) MPIU_ERR_POP (mpi_errno); */

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_COLL_BARRIER_INIT);
    return mpi_errno;
}
