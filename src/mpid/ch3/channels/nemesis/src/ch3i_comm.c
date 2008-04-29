/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include <sched.h>
#define BUSY_WAIT() sched_yield()

#define NULL_CONTEXT_ID -1

static int barrier (MPID_Comm *comm_ptr);
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

/* find_local_and_external -- from the list of processes in comm,
   builds a list of local processes, i.e., processes on this same
   node, and a list of external processes, i.e., one process from each
   node.
*/

#undef FUNCNAME
#define FUNCNAME find_local_and_external
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int find_local_and_external (MPID_Comm *comm, int *local_size_p, int *local_rank_p, int **local_ranks_p,
                                    int *external_size_p, int *external_rank_p, int **external_ranks_p)
{
    int mpi_errno = MPI_SUCCESS;
    int *nodes;
    int external_size;
    int external_rank;
    int *external_ranks;
    int local_size;
    int local_rank;
    int *local_ranks;
    int i;
    int node_id; /* this is set to -1 for a proc which is not using shared memory (e.g., spawned proc) */
    int my_node_id;
    MPIU_CHKLMEM_DECL(1);
    MPIU_CHKPMEM_DECL(2);

    /* Scan through the list of processes in comm and add one
       process from each node to the list of "external" processes.  We
       add the first process we find from each node.  nodes[] is an
       array where we keep track of whether we have already added that
       node to the list. */

    MPIU_CHKPMEM_MALLOC (external_ranks, int *, sizeof(int) * MPID_nem_mem_region.num_nodes, mpi_errno, "external_ranks");
    MPIU_CHKPMEM_MALLOC (local_ranks, int *, sizeof(int) * comm->remote_size, mpi_errno, "local_ranks");
    MPIU_CHKLMEM_MALLOC (nodes, int *, sizeof(int) * MPID_nem_mem_region.num_nodes, mpi_errno, "nodes");

    for (i = 0; i < MPID_nem_mem_region.num_nodes; ++i)
        nodes[i] = 0;

    external_size = 0;
    my_node_id = ((MPIDI_CH3I_VC *)comm->vcr[comm->rank]->channel_private)->node_id;
    local_size = 0;
    local_rank = -1;
    external_rank = -1;

    for (i = 0; i < comm->remote_size; ++i)
    {
        node_id = ((MPIDI_CH3I_VC *)comm->vcr[i]->channel_private)->node_id;

        /* build list of external processes */
        if (node_id == -1 || nodes[node_id] == 0)
        {
            if (i == comm->rank)
                external_rank = external_size;
            if (node_id != -1)
                nodes[node_id] = 1;
            external_ranks[external_size] = i;
            ++external_size;
        }

        /* build list of local processes */
        if (node_id != -1 && node_id == my_node_id)
        {
             if (i == comm->rank)
                 local_rank = local_size;
             local_ranks[local_size] = i;
            ++local_size;
        }
    }

    *local_size_p = local_size;
    *local_rank_p = local_rank;
    *local_ranks_p =  MPIU_Realloc (local_ranks, sizeof(int) * local_size);
    MPIU_ERR_CHKANDJUMP (*local_ranks_p == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem2");

    *external_size_p = external_size;
    *external_rank_p = external_rank;
    *external_ranks_p = MPIU_Realloc (external_ranks, sizeof(int) * external_size);
    MPIU_ERR_CHKANDJUMP (*external_ranks_p == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem2");

    MPIU_CHKPMEM_COMMIT();

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


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

    mpi_errno = find_local_and_external (comm, &comm->ch.local_size, &comm->ch.local_rank, &comm->ch.local_ranks,
                                         &comm->ch.external_size, &comm->ch.external_rank, &comm->ch.external_ranks);
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
    if (comm->ch.barrier_vars && MPID_NEM_FETCH_AND_DEC (&comm->ch.barrier_vars->usage_cnt) == 1)
    {
	MPID_NEM_WRITE_BARRIER();
	comm->ch.barrier_vars->context_id = NULL_CONTEXT_ID;
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
	c = MPID_NEM_CAS_INT (&MPID_nem_mem_region.barrier_vars[i].context_id, NULL_CONTEXT_ID, comm->context_id);
        if (c == NULL_CONTEXT_ID || c == comm->context_id)
        {
            *vars = &MPID_nem_mem_region.barrier_vars[i];
	    MPID_NEM_WRITE_BARRIER();
            MPID_NEM_ATOMIC_INC (&(*vars)->usage_cnt);
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
static int barrier (MPID_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    volatile MPID_nem_barrier_vars_t *barrier_vars;
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

        sense = barrier_vars->sig;
	MPID_NEM_READ_BARRIER();

        prev = MPID_NEM_FETCH_AND_INC (&barrier_vars->cnt);
        if (prev == local_size - 1)
        {
            barrier_vars->cnt = 0;
            MPID_NEM_WRITE_BARRIER();
            barrier_vars->sig = 1 - sense;
        }
        else
        {
            while (barrier_vars->sig == sense)
                BUSY_WAIT();
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
            while (barrier_vars->sig0 == 0)
                BUSY_WAIT();

        /* now do a barrier with external processes */
        mpi_errno = msg_barrier (comm_ptr, external_rank, external_size, external_ranks);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        /* reset ctr and release local procs */
        if (local_size > 1)
        {
            barrier_vars->sig0 = 0;
            barrier_vars->cnt = 0;
	    MPID_NEM_WRITE_BARRIER();
	    barrier_vars->sig = 1 - barrier_vars->sig;
        }
    }
    else
    {
        /* just do the local barrier -- Decrement a counter.  If
           counter is 1 (i.e., only root is left), set sig0 to signal
           root.  Then, wait on signal variable. */
        int prev;
        int sense;
        sense = barrier_vars->sig;
	MPID_NEM_READ_BARRIER();

        prev = MPID_NEM_FETCH_AND_INC (&barrier_vars->cnt);
        if (prev == local_size - 2)  /* - 2 because it's the value before we added 1 and we're not waiting for root */
	{
	    MPID_NEM_WRITE_BARRIER();
	    barrier_vars->sig0 = 1;
	}

        while (barrier_vars->sig == sense)
            BUSY_WAIT();
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
            barrier_region[i].context_id = NULL_CONTEXT_ID;
            barrier_region[i].usage_cnt = 0;
            barrier_region[i].cnt = 0;
            barrier_region[i].sig0 = 0;
            barrier_region[i].sig = 0;
        }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_BARRIER_VARS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier_vars_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_coll_barrier_init ()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_COLL_BARRIER_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_COLL_BARRIER_INIT);

/*     mpi_errno = MPIDI_CH3I_comm_create (MPIR_Process.comm_world); */
/*     if (mpi_errno) MPIU_ERR_POP (mpi_errno); */

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_COLL_BARRIER_INIT);
    return mpi_errno;
}
