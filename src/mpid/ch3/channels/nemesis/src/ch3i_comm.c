/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#undef utarray_oom
#define utarray_oom() do { goto fn_oom; } while (0)
#include "mpiu_utarray.h"

#define NULL_CONTEXT_ID -1

static int barrier (MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag);
static int alloc_barrier_vars (MPID_Comm *comm, MPID_nem_barrier_vars_t **vars);

UT_array *coll_fns_array = NULL;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_comm_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_comm_create(MPID_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMM_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_COMM_CREATE);

#ifndef ENABLED_SHM_COLLECTIVES
    goto fn_exit;
#endif
    
    /* set up intranode barrier iff this is an intranode communicator */
    if (comm->hierarchy_kind == MPID_HIERARCHY_NODE) {
        MPID_Collops *cf, **cf_p;
        comm->dev.ch.barrier_vars = NULL;

        /* We can't use a static coll_fns override table for our collectives
           because we store a pointer to the previous coll_fns in our coll_fns
           table and different communicators can potentially have different
           coll_fns.  In the worst case, we may need one coll_fns table for each
           communicator, in practice, however, we don't expect to need more than
           a few.

           Warning if you are copying this code: This code is not tested well
           for the case where multiple coll_fns are needed because we don't
           currently have a use case for it.
        */
        cf_p = NULL;
        while ( (cf_p = (MPID_Collops **)utarray_next(coll_fns_array, cf_p)) ) {
            /* we can reuse a coll_fns table if the prev_coll_fns pointer is
               the same as the coll_fns of this communicator */
            if ((*cf_p)->prev_coll_fns == comm->coll_fns) {
                comm->coll_fns = *cf_p;
                ++comm->coll_fns->ref_count;
                goto fn_exit;
            }
        }

        /* allocate and init new coll_fns table */
        MPIU_CHKPMEM_MALLOC(cf, MPID_Collops *, sizeof(*cf), mpi_errno, "cf");
        *cf = *comm->coll_fns;
        cf->ref_count = 1;
        cf->Barrier = barrier;
        cf->prev_coll_fns = comm->coll_fns;
        utarray_push_back(coll_fns_array, &cf);
        
        /* replace coll_fns table */
        comm->coll_fns = cf;
    }
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_COMM_CREATE);
    return mpi_errno;
 fn_oom: /* out-of-memory handler for utarray operations */
    MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "utarray");
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_comm_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_comm_destroy(MPID_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);
#ifndef ENABLED_SHM_COLLECTIVES
    goto fn_exit;
#endif
    
    if (comm->hierarchy_kind == MPID_HIERARCHY_NODE) {
        MPID_Collops *cf = comm->coll_fns;

        /* replace previous coll_fns table */
        comm->coll_fns = cf->prev_coll_fns;

        /* free coll_fns if it's no longer used */
        --cf->ref_count;
        if (cf->ref_count == 0) {
            utarray_erase(coll_fns_array, utarray_eltidx(coll_fns_array, cf), 1);
            MPIU_Free(cf);
        }
            
        if (comm->dev.ch.barrier_vars && OPA_fetch_and_decr_int(&comm->dev.ch.barrier_vars->usage_cnt) == 1) {
            OPA_write_barrier();
            OPA_store_int(&comm->dev.ch.barrier_vars->context_id, NULL_CONTEXT_ID);
        }
    }
    
 fn_exit: ATTRIBUTE((unused))
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME alloc_barrier_vars
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int alloc_barrier_vars (MPID_Comm *comm, MPID_nem_barrier_vars_t **vars)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int c;

    /* FIXME:  This has a serious design bug.  It assumes that context ids are
       globally unique (i.e., that if two processes have communicators with the
       same context id, they're the same communicator), but this is not true.
       This may result in two different communicators using the same
       barier_vars.  This code is being left in for now as an example of how to
       override collective operations. */
    MPIU_Assert(0);

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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int barrier(MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_barrier_vars_t *barrier_vars;
    int prev;
    int sense;
    
    MPIU_Assert(comm_ptr->hierarchy_kind == MPID_HIERARCHY_NODE);
    
    /* Trivial barriers return immediately */
    if (comm_ptr->local_size == 1)
        return MPI_SUCCESS;

    /* Only one collective operation per communicator can be active at any
       time */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER (comm_ptr);

    if (comm_ptr->dev.ch.barrier_vars == NULL) {
        mpi_errno = alloc_barrier_vars (comm_ptr, &comm_ptr->dev.ch.barrier_vars);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);

        if (comm_ptr->dev.ch.barrier_vars == NULL) {
            /* no barrier_vars left -- revert to default barrier. */
            /* FIXME: need a better solution here.  e.g., allocate
               some barrier_vars on the first barrier for the life of
               the communicator (as is the case now), others must be
               allocated for each barrier, then released.  If we run
               out of barrier_vars after that, then use msg_barrier.
            */
            if (comm_ptr->coll_fns->prev_coll_fns->Barrier != NULL) {
                mpi_errno = comm_ptr->coll_fns->prev_coll_fns->Barrier(comm_ptr, errflag);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            } else {
                mpi_errno = MPIR_Barrier_intra(comm_ptr, errflag);
                if (mpi_errno) MPIR_ERR_POP (mpi_errno);
            }
            goto fn_exit;
        }
    }

    barrier_vars = comm_ptr->dev.ch.barrier_vars;

    sense = OPA_load_int(&barrier_vars->sig);
    OPA_read_barrier();

    prev = OPA_fetch_and_incr_int(&barrier_vars->cnt);
    if (prev == comm_ptr->local_size - 1)
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

 fn_exit:
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_barrier_vars_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
#define FUNCNAME nem_coll_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int nem_coll_finalize(void *param ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEM_COLL_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_NEM_COLL_FINALIZE);

    utarray_free(coll_fns_array);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEM_COLL_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_coll_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_coll_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_COLL_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_COLL_INIT);

    utarray_new(coll_fns_array, &ut_ptr_icd);
    MPIR_Add_finalize(nem_coll_finalize, NULL, MPIR_FINALIZE_CALLBACK_PRIO-1);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_COLL_INIT);
    return mpi_errno;
 fn_oom: /* out-of-memory handler for utarray operations */
    MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "utarray");
 fn_fail:
    goto fn_exit;
}

