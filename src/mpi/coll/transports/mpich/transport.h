/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef MPICHTRANSPORT_H_INCLUDED
#define MPICHTRANSPORT_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_test(MPIC_MPICH_sched_t * sched);
/* Internal transport function for issuing a vertex in the graph */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_issue_vtx(int, MPIC_MPICH_vtx_t *, MPIC_MPICH_sched_t *);

/* Transport function for allocating memory */
MPL_STATIC_INLINE_PREFIX void *MPIC_MPICH_allocate_mem(size_t size)
{
    return MPL_malloc(size);
}

/* Transport function for free'ing memory */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_free_mem(void *ptr)
{
    MPIR_Assertp(ptr != NULL);
    MPL_free(ptr);
}

/* Internal transport function. This function is called whenever a vertex completes its execution.
 * It notifies the outgoing vertices of the completed vertex about its completion by decrementing
 * the number of their number of unfinished dependencies */
/* This is set to be static inline because compiler cannot build this function with MPL_STATIC_INLINE_PREFIX
 * complaining that this is a non-inlineable function */
static inline void MPIC_MPICH_decrement_num_unfinished_dependecies(MPIC_MPICH_vtx_t * vtxp,
                                                                   MPIC_MPICH_sched_t * sched)
{
    int i;

    /* Get the list of outgoing vertices */
    MPIC_MPICH_int_array *outvtcs = &vtxp->outvtcs;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Number of outgoing vertices of %d = %d\n", vtxp->id, outvtcs->used));
    /* for each outgoing vertex of vertex *vtxp, decrement number of unfinished dependencies */
    for (i = 0; i < outvtcs->used; i++) {
        int num_unfinished_dependencies =
            --(sched->vtcs[outvtcs->array[i]].num_unfinished_dependencies);
        /* if all dependencies of the outgoing vertex are complete, issue the vertex */
        if (num_unfinished_dependencies == 0) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Issuing vertex number %d\n", outvtcs->array[i]));
            MPIC_MPICH_issue_vtx(outvtcs->array[i], &sched->vtcs[outvtcs->array[i]], sched);
        }
    }
}

/* Internal transport function to record completion of a vertex and take appropriate actions */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_record_vtx_completion(MPIC_MPICH_vtx_t * vtxp,
                                                  MPIC_MPICH_sched_t * sched)
{
    vtxp->state = MPIC_MPICH_STATE_COMPLETE;
    sched->num_completed++;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Number of completed vertices = %d\n", sched->num_completed));

    /* update outgoing vertices about this vertex completion */
    MPIC_MPICH_decrement_num_unfinished_dependecies(vtxp, sched);
}

/* Internal transport function to record issue of a veretex */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_record_vtx_issue(MPIC_MPICH_vtx_t * vtxp, MPIC_MPICH_sched_t * sched)
{
    vtxp->state = MPIC_MPICH_STATE_ISSUED;

    /* Update the linked list of issued vertices */

    /* If this is the first issued vertex in the linked list */
    if (sched->issued_head == NULL) {
        sched->issued_head = sched->last_issued = vtxp;
        sched->last_issued->next_issued = sched->vtx_iter;
    }
    /* If this is an already issued vertex that is part of the old linked list */
    else if (sched->last_issued->next_issued == vtxp) {
        sched->last_issued = vtxp;
    }
    /* Else, this is a newly issued vertex - insert it between last_issued and vtx_iter */
    else {
        sched->last_issued->next_issued = vtxp;
        vtxp->next_issued = sched->vtx_iter;
        sched->last_issued = vtxp;
    }

#ifdef MPIC_DEBUG
    /* print issued vertex list */
    MPIC_MPICH_vtx_t *vtx = sched->issued_head;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Issued vertices list: "));
    while (vtx) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"%d ", vtx->id));
        vtx = vtx->next_issued;
    }
    /* print the current location of vertex iterator */
    if (sched->vtx_iter)
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,", vtx_iter: %d\n", sched->vtx_iter->id));
    else
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"\n"));
#endif

}

/* Transport initialization function */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_init()
{
    MPIC_global_instance.tsp_mpich.control_dt = MPI_BYTE;
    return 0;
}

/* Transport function to store a schedule */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_save_schedule(MPIC_MPICH_comm_t * comm, void *key, int len, void *s)
{
    MPIC_add_sched(&(comm->sched_db), key, len, s);
}

/* Internal transport function to reset issued vertex list */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_reset_issued_list(MPIC_MPICH_sched_t * sched)
{
    int i, nvtcs;

    sched->issued_head = NULL;
    /* If the schedule is being reused, reset only used vtcs because
     * it is known that only that many vertices will ever be used.
     * Else it is a new schedule being generated and therefore
     * reset all the vtcs* */
    nvtcs = (sched->total == 0) ? sched->max_vtcs : sched->total;
    for (i = 0; i < nvtcs; i++) {
        sched->vtcs[i].next_issued = NULL;
    }

    sched->vtx_iter = NULL;
}

/* Transport function to initialize a new schedule */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_sched_init(MPIC_MPICH_sched_t * sched, int tag)
{
    sched->total = 0;
    sched->num_completed = 0;
    sched->last_wait = -1;
    sched->tag = tag;
    sched->max_vtcs = MPIC_MPICH_MAX_TASKS;
    sched->max_edges_per_vtx = MPIC_MPICH_MAX_EDGES;

    /* allocate memory for storing vertices */
    sched->vtcs = MPIC_MPICH_allocate_mem(sizeof(MPIC_MPICH_vtx_t) * sched->max_vtcs);
    /* initialize array for storing memory buffer addresses */
    sched->buf_array.array = (void **) MPIC_MPICH_allocate_mem(sizeof(void *) * sched->max_vtcs);
    sched->buf_array.size = sched->max_vtcs;
    sched->buf_array.used = 0;

    /* reset issued vertex list */
    MPIC_MPICH_reset_issued_list(sched);
}

/* Transport function to reset an existing schedule */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_sched_reset(MPIC_MPICH_sched_t * sched, int tag)
{
    int i;

    sched->num_completed = 0;
    sched->tag = tag;   /* set new tag value */

    for (i = 0; i < sched->total; i++) {
        MPIC_MPICH_vtx_t *vtx = &sched->vtcs[i];
        vtx->state = MPIC_MPICH_STATE_INIT;
        /* reset the number of unfinished dependencies to be the number of incoming vertices */
        vtx->num_unfinished_dependencies = vtx->invtcs.used;
        /* recv_reduce tasks need to be set as not done */
        if (vtx->kind == MPIC_MPICH_KIND_RECV_REDUCE)
            vtx->nbargs.recv_reduce.done = 0;
        /* reset progress of multicast operation */
        if(vtx->kind == MPIC_MPICH_KIND_MULTICAST)
            vtx->nbargs.multicast.last_complete = -1;
    }
    /* Reset issued vertex list to NULL.
     * **TODO: Check if this is really required as the vertex linked list
     * might be NULL implicitly at the end of the previous schedule use
     * */
    MPIC_MPICH_reset_issued_list(sched);
}

/* Transport function to get schedule (based on the key) if it exists, else return a new schedule */
MPL_STATIC_INLINE_PREFIX MPIC_MPICH_sched_t *MPIC_MPICH_get_schedule(MPIC_MPICH_comm_t * comm,
                                                        void *key, int len, int tag, int *is_new)
{
    MPIC_MPICH_sched_t *sched = MPIC_get_sched(comm->sched_db, key, len);
    if (sched) {        /* schedule already exists */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Schedule is loaded from the database[%p]\n", sched));
        *is_new = 0;
        /* reset the schedule for reuse */
        MPIC_MPICH_sched_reset(sched, tag);
    } else {
        *is_new = 1;
        sched = MPIC_MPICH_allocate_mem(sizeof(MPIC_MPICH_sched_t));
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"New schedule is created:[%p]\n", sched));
        /* initialize the newly created schedule */
        MPIC_MPICH_sched_init(sched, tag);
    }
    return sched;
}

/* Transport function to tell that the schedule generation is now complete */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_sched_commit(MPIC_MPICH_sched_t * sched)
{
}

/* Transport function to tell that the schedule can now be freed
 * and can also be removed from the database. We are not doing
 * anything here because the schedule is stored in the database
 * can be reused in future */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_sched_finalize(MPIC_MPICH_sched_t * sched)
{
}

/* Transport function to get commutativity of the operation */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_opinfo(MPIC_MPICH_op_t op, int *is_commutative)
{
    MPIR_Op *op_ptr;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
        *is_commutative = 1;
    else {
        MPIR_Op_get_ptr(op, op_ptr);

        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            *is_commutative = 0;
        else
            *is_commutative = 1;
    }
}

/* Transport function to tell if the buffer is in_place - used by some
 * collective operations */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_isinplace(const void *buf)
{
    if (buf == MPI_IN_PLACE)
        return 1;

    return 0;
}

/* Transport function to get datatype information */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_dtinfo(MPIC_MPICH_dt_t dt,
                                   int *iscontig,
                                   size_t * size, size_t * out_extent, size_t * lower_bound)
{
    MPI_Aint true_lb, extent, true_extent, type_size;

    MPIR_Datatype_get_size_macro(dt, type_size);
    MPIR_Type_get_true_extent_impl(dt, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(dt, extent);
    MPIR_Datatype_is_contig(dt, iscontig);

    *size = type_size;
    *out_extent = MPL_MAX(extent, true_extent);
    *lower_bound = true_lb;
}

/* Transport function to add reference to a datatype */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_addref_dt(MPIC_MPICH_dt_t dt, int up)
{
    MPIR_Datatype *dt_ptr;

    if (HANDLE_GET_KIND(dt) != HANDLE_KIND_BUILTIN) {
        MPIR_Datatype_get_ptr(dt, dt_ptr);

        if (up)
            MPIR_Datatype_add_ref(dt_ptr);
        else
            MPIR_Datatype_release(dt_ptr);
    }
}

/* Internal transport function to allocate memory associated with a graph vertex */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_allocate_vtx(MPIC_MPICH_vtx_t * vtx, int invtcs_array_size,
                                         int outvtcs_array_size)
{
    /* allocate memory for storing incoming and outgoing vertices */
    vtx->invtcs.array = MPIC_MPICH_allocate_mem(sizeof(int) * invtcs_array_size);
    vtx->invtcs.size = invtcs_array_size;
    vtx->outvtcs.array = MPIC_MPICH_allocate_mem(sizeof(int) * outvtcs_array_size);
    vtx->outvtcs.size = outvtcs_array_size;
}

/* Internal transport function to initialize a graph vertex */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_init_vtx(MPIC_MPICH_sched_t * sched, MPIC_MPICH_vtx_t * vtx, int id)
{
    /* allocate memory for storing incoming and outgoing edges */
    MPIC_MPICH_allocate_vtx(vtx, sched->max_edges_per_vtx, sched->max_edges_per_vtx);
    vtx->invtcs.used = 0;
    vtx->outvtcs.used = 0;

    vtx->state = MPIC_MPICH_STATE_INIT;
    vtx->id = id;
    vtx->num_unfinished_dependencies = 0;
}

/* Internal transport function to free the memory associated with a vertex */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_free_vtx(MPIC_MPICH_vtx_t * vtx)
{
    MPIC_MPICH_free_mem(vtx->invtcs.array);
    MPIC_MPICH_free_mem(vtx->outvtcs.array);
}

/* Internal transport function to add list of integers to an integer array (MPIC_MPICH_int_array)*/
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_add_elems_int_array(MPIC_MPICH_int_array * in, int n_elems, int *elems)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"In add_elems_int_array, in->used: %d, in->size: %d\n", in->used, in->size));
    if (in->used + n_elems > in->size) {        /* not sufficient memory */
        int *old_array = in->array;

        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Increasing size of the int_array\n"));

        in->size = MPL_MAX(2 * in->size, in->used + n_elems);
        /* reallocate array */
        in->array = (int *) MPIC_MPICH_allocate_mem(sizeof(int) * in->size);
        /* copy old elements */
        memcpy(in->array, old_array, sizeof(int) * in->used);
        /* free old array */
        MPIC_MPICH_free_mem(old_array);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Adding %d elems to int_array\n", n_elems));
    /* add new elements */
    memcpy(in->array + in->used, elems, sizeof(int) * n_elems);
    in->used += n_elems;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Done adding %d elems to int_array\n", n_elems));
}

/* Internal transport function to add incoming vertices of a vertex.
 * This vertex sets the incoming vertices to vtx and also adds vtx to the
 * outgoing vertex list of the vertives in invtcs.
 * NOTE: This function should only be called when a new vertex is added to the groph */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_add_vtx_dependencies(MPIC_MPICH_sched_t * sched, int vtx_id,
                                                 int n_invtcs, int *invtcs)
{
    int i;
    MPIC_MPICH_int_array *outvtcs;
    MPIC_MPICH_vtx_t *vtx = &sched->vtcs[vtx_id];
    MPIC_MPICH_int_array *in = &vtx->invtcs;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Updating invtcs of vtx %d, kind %d, in->used %d, n_invtcs %d\n", vtx_id,
             vtx->kind, in->used, n_invtcs));
    /* insert the incoming edges */
    MPIC_MPICH_add_elems_int_array(in, n_invtcs, invtcs);

    /* update the list of outgoing vertices of the incoming vertices */
    for (i = 0; i < n_invtcs; i++) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"invtx: %d\n", invtcs[i]));
        outvtcs = &sched->vtcs[invtcs[i]].outvtcs;
        MPIC_MPICH_add_elems_int_array(outvtcs, 1, &vtx_id);

        /* increment num_unfinished_dependencies only is the incoming vertex is not complete yet */
        if (sched->vtcs[invtcs[i]].state != MPIC_MPICH_STATE_COMPLETE)
            vtx->num_unfinished_dependencies++;
    }

    /* check if there was any MPIC_MPICH_wait operation and add appropriate dependencies.
     * TSP_wait function does not return a vertex id, so the application will never explicity
     * specify a dependency on it, the transport has to make sure that the dependency
     * on the wait operation is met */
    if (sched->last_wait != -1 && sched->last_wait != vtx_id) {
        /* add the last wait vertex as an incoming vertex to vtx */
        MPIC_MPICH_add_elems_int_array(in, 1, &(sched->last_wait));

        /* add vtx as outgoing vtx of last_wait */
        outvtcs = &sched->vtcs[sched->last_wait].outvtcs;
        MPIC_MPICH_add_elems_int_array(outvtcs, 1, &vtx_id);

        if (sched->vtcs[sched->last_wait].state != MPIC_MPICH_STATE_COMPLETE)
            vtx->num_unfinished_dependencies++;
    }
}

/* Internal transport function to get a new vertex in the graph */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_get_new_vtx(MPIC_MPICH_sched_t * sched, MPIC_MPICH_vtx_t ** vtx)
{
    /* If all vertices have been used */
    if (sched->total == sched->max_vtcs) {      /* increase vertex array size */
        int old_size = sched->max_vtcs;
        MPIC_MPICH_vtx_t *old_vtcs = sched->vtcs;
        sched->max_vtcs *= 2;
        /* allocate new array */
        sched->vtcs = MPIC_MPICH_allocate_mem(sizeof(MPIC_MPICH_vtx_t) * sched->max_vtcs);
        /* copy vertices from old array to new array */
        memcpy(sched->vtcs, old_vtcs, sizeof(MPIC_MPICH_vtx_t) * old_size);
        int i;
        for (i = 0; i < old_size; i++) {
            /* Reset pointer to point to new memory location in case of recv_reduce */
            MPIC_MPICH_vtx_t *vtxp = &sched->vtcs[i];
            if(vtxp->kind == MPIC_MPICH_KIND_RECV_REDUCE){
               vtxp->nbargs.recv_reduce.vtxp = vtxp;
            }
            /*copy incoming/outgoing vertices from old array to new array */
            MPIC_MPICH_allocate_vtx(&sched->vtcs[i], old_vtcs[i].invtcs.size,
                                    old_vtcs[i].outvtcs.size);
            /* used values get copied in the memcpy above */
            memcpy(sched->vtcs[i].invtcs.array, old_vtcs[i].invtcs.array,
                   sizeof(int) * old_vtcs[i].invtcs.used);
            memcpy(sched->vtcs[i].outvtcs.array, old_vtcs[i].outvtcs.array,
                   sizeof(int) * old_vtcs[i].outvtcs.used);
            /* free the memory allocated by the old vertex */
            MPIC_MPICH_free_vtx(&old_vtcs[i]);
        }
        /* free the arary of old vertices */
        MPIC_MPICH_free_mem(old_vtcs);
    }
    /* return last unused vertex */
    *vtx = &sched->vtcs[sched->total];
    return sched->total++;      /* return vertex id */
}

/* Transport function that adds a no op vertex in the graph that has
 * all the vertices posted before it as incoming vertices */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_fence(MPIC_MPICH_sched_t * sched)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [fence] total=%d \n", sched->total));

    MPIC_MPICH_vtx_t *vtxp;
    int *invtcs, i, n_invtcs = 0, vtx_id;

    vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);

    vtxp->kind = MPIC_MPICH_KIND_FENCE;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);

    invtcs = (int *) MPIC_MPICH_allocate_mem(sizeof(int) * vtx_id);
    /* record incoming vertices */
    for (i = vtx_id - 1; i >= 0; i--) {
        if (sched->vtcs[i].kind == MPIC_MPICH_KIND_WAIT)
            /* no need to record this and any vertex before wait.
             * Dependency on the last wait call will be added by
             * the subsequent call to MPIC_MPICH_add_vtx_dependencies function */
            break;
        else {
            invtcs[vtx_id - 1 - i] = i;
            n_invtcs++;
        }
    }

    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);
    MPL_free(invtcs);
    return vtx_id;
}

/* Transport function that adds a no op vertex in the graph with specified dependencies */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_wait_for(MPIC_MPICH_sched_t * sched, int nvtcs, int *vtcs)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [wait_for] total=%d \n", sched->total));
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);

    vtxp->kind = MPIC_MPICH_KIND_NOOP;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);

    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, nvtcs, vtcs);
    return vtx_id;
}

/* MPIC_MPICH_wait waits for all the operations posted before it to complete
* before issuing any operations posted after it. This is useful in composing
* multiple schedules, for example, allreduce can be written as
* COLL_sched_reduce(s)
* MPIC_MPICH_wait(s)
* COLL_sched_bcast(s)
* This is different from the fence operation in the sense that fence requires
* a vertex to post dependencies on it while MPIC_MPICH_wait is used internally
* by the transport to add it as a dependency to any operations poster after it
*/
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_wait(MPIC_MPICH_sched_t * sched)
{
    int wait_id;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Scheduling a TSP_wait\n"));
    /* wait operation is an extension to fence, so we can resuse the fence call */
    wait_id = MPIC_MPICH_fence(sched);
    /* change the vertex kind from FENCE to WAIT */
    sched->vtcs[wait_id].kind = MPIC_MPICH_KIND_WAIT;
    sched->last_wait = wait_id;
    return wait_id;
}

/* Transport function to add reference to an operation */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_addref_op(MPIC_MPICH_op_t op, int up)
{
    MPIR_Op *op_ptr;

    if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
        MPIR_Op_get_ptr(op, op_ptr);

        if (up)
            MPIR_Op_add_ref(op_ptr);
        else
            MPIR_Op_release(op_ptr);
    }
}

/* Transport function to schedule addition of a reference to a datatype */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_addref_dt_nb(MPIC_MPICH_dt_t dt, int up,
                                        MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);
    vtxp->kind = MPIC_MPICH_KIND_ADDREF_DT;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.addref_dt.dt = dt;
    vtxp->nbargs.addref_dt.up = up;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [addref_dt]\n", vtx_id));
    return vtx_id;
}

/* Transport function to schedule addition of a reference to a datatype */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_addref_op_nb(MPIC_MPICH_op_t op, int up,
                                        MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);
    vtxp->kind = MPIC_MPICH_KIND_ADDREF_OP;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.addref_op.op = op;
    vtxp->nbargs.addref_op.up = up;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [addref_op]\n", vtx_id));
    return vtx_id;
}

/* Transport function to schedule a send task */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_send(const void *buf,
                                int count,
                                MPIC_MPICH_dt_t dt,
                                int dest,
                                int tag,
                                MPIC_MPICH_comm_t * comm,
                                MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);
    sched->tag = tag;
    vtxp->kind = MPIC_MPICH_KIND_SEND;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.sendrecv.buf = (void *) buf;
    vtxp->nbargs.sendrecv.count = count;
    vtxp->nbargs.sendrecv.dt = dt;
    vtxp->nbargs.sendrecv.dest = dest;
    vtxp->nbargs.sendrecv.comm = comm;
    /* second request is not used */
    vtxp->mpid_req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [send]\n", vtx_id));
    return vtx_id;
}

MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_multicast(const void *buf,
                                     int count,
                                     MPIC_MPICH_dt_t dt,
                                     int* destinations,
                                     int num_destinations,
                                     int tag,
                                     MPIC_MPICH_comm_t * comm,
                                     MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);
    sched->tag = tag;
    vtxp->kind = MPIC_MPICH_KIND_MULTICAST;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);
    /* store the arguments */
    vtxp->nbargs.multicast.buf = (void *) buf;
    vtxp->nbargs.multicast.count = count;
    vtxp->nbargs.multicast.dt = dt;
    vtxp->nbargs.multicast.num_destinations = num_destinations;
    vtxp->nbargs.multicast.destinations = (int *) MPIC_MPICH_allocate_mem(sizeof(int)*num_destinations);
    memcpy(vtxp->nbargs.multicast.destinations, destinations, sizeof(int)*num_destinations);

    vtxp->nbargs.multicast.comm = comm;
    vtxp->nbargs.multicast.mpir_req = (struct MPIR_Request**)MPIC_MPICH_allocate_mem(sizeof(struct MPIR_Request*)*num_destinations);
    vtxp->nbargs.multicast.last_complete = -1;
    /* set unused request pointers to NULL */
    vtxp->mpid_req[0] = NULL;
    vtxp->mpid_req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [send]\n", vtx_id));
    return vtx_id;
}

/* Transport function to schedule a send_accumulate task */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_send_accumulate(const void *buf,
                                           int count,
                                           MPIC_MPICH_dt_t dt,
                                           MPIC_MPICH_op_t op,
                                           int dest,
                                           int tag,
                                           MPIC_MPICH_comm_t * comm,
                                           MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);

    sched->tag = tag;
    vtxp->kind = MPIC_MPICH_KIND_SEND;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.sendrecv.buf = (void *) buf;
    vtxp->nbargs.sendrecv.count = count;
    vtxp->nbargs.sendrecv.dt = dt;
    vtxp->nbargs.sendrecv.dest = dest;
    vtxp->nbargs.sendrecv.comm = comm;
    /* second request is not used */
    vtxp->mpid_req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [send_accumulate]\n", vtx_id));
    return vtx_id;
}

/* Transport function to schedule a recv task */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_recv(void *buf,
                                int count,
                                MPIC_MPICH_dt_t dt,
                                int source,
                                int tag,
                                MPIC_MPICH_comm_t * comm,
                                MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);

    sched->tag = tag;
    vtxp->kind = MPIC_MPICH_KIND_RECV;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* record the arguments */
    vtxp->nbargs.sendrecv.buf = buf;
    vtxp->nbargs.sendrecv.count = count;
    vtxp->nbargs.sendrecv.dt = dt;
    vtxp->nbargs.sendrecv.dest = source;
    vtxp->nbargs.sendrecv.comm = comm;
    vtxp->mpid_req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [recv]\n", vtx_id));
    return vtx_id;
}


/* Internal transport function to do the reduce in the recv_reduce call */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_queryfcn(void *data, MPI_Status * status)
{
    MPIC_MPICH_recv_reduce_arg_t *rr = (MPIC_MPICH_recv_reduce_arg_t *) data;
    /* Check if recv has completed (mpid_req[0]==NULL) and reduce has not already been done (!rr->done) */
    if (rr->vtxp->mpid_req[0] == NULL && !rr->done) {
        MPI_Datatype dt = rr->datatype;
        MPI_Op op = rr->op;

        if (rr->flags == -1 || rr->flags & MPIC_FLAG_REDUCE_L) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (recv_reduce L) complete to %p\n", rr->inoutbuf));

            MPIR_Reduce_local_impl(rr->inbuf, rr->inoutbuf, rr->count, dt, op);
        } else {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (recv_reduce R) complete to %p\n", rr->inoutbuf));

            MPIR_Reduce_local_impl(rr->inoutbuf, rr->inbuf, rr->count, dt, op);
            MPIR_Localcopy(rr->inbuf, rr->count, dt, rr->inoutbuf, rr->count, dt);
        }

        MPIR_Grequest_complete_impl(rr->vtxp->mpid_req[1]);
        rr->done = 1;   /* recv_reduce is complete now */
    }

    /* TODO: Check if this has to be done everytime */
    status->MPI_SOURCE = MPI_UNDEFINED;
    status->MPI_TAG = MPI_UNDEFINED;
    MPI_Status_set_cancelled(status, 0);
    MPI_Status_set_elements(status, MPI_BYTE, 0);
    return MPI_SUCCESS;
}

/* Transport function to schedule a recv_reduce call */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_recv_reduce(void *buf,
                                       int count,
                                       MPIC_MPICH_dt_t datatype,
                                       MPIC_MPICH_op_t op,
                                       int source,
                                       int tag,
                                       MPIC_MPICH_comm_t * comm,
                                       uint64_t flags,
                                       MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    int iscontig;
    size_t type_size, out_extent, lower_bound;
    MPIC_MPICH_vtx_t *vtxp;

    /* assign a vertex */
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);
    sched->tag = tag;
    vtxp->kind = MPIC_MPICH_KIND_RECV_REDUCE;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    MPIC_MPICH_dtinfo(datatype, &iscontig, &type_size, &out_extent, &lower_bound);
    vtxp->nbargs.recv_reduce.inbuf = MPIC_MPICH_allocate_mem(count * out_extent);
    vtxp->nbargs.recv_reduce.inoutbuf = buf;
    vtxp->nbargs.recv_reduce.count = count;
    vtxp->nbargs.recv_reduce.datatype = datatype;
    vtxp->nbargs.recv_reduce.op = op;
    vtxp->nbargs.recv_reduce.source = source;
    vtxp->nbargs.recv_reduce.comm = comm;
    vtxp->nbargs.recv_reduce.vtxp = vtxp;
    vtxp->nbargs.recv_reduce.done = 0;
    vtxp->nbargs.recv_reduce.flags = flags;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [recv_reduce]\n", vtx_id));

    return vtx_id;
}

/* Transport function to get rank in communicator */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_rank(MPIC_MPICH_comm_t * comm)
{
    return comm->mpid_comm->rank;
}

/* Transport function to get size of the communicator */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_size(MPIC_MPICH_comm_t * comm)
{
    return comm->mpid_comm->local_size;
}

/* Transport function to schedule a local reduce */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_reduce_local(const void *inbuf,
                                        void *inoutbuf,
                                        int count,
                                        MPIC_MPICH_dt_t datatype,
                                        MPIC_MPICH_op_t operation,
                                        uint64_t flags,
                                        MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);

    /* assign a new vertex */
    vtxp->kind = MPIC_MPICH_KIND_REDUCE_LOCAL;
    vtxp->state = MPIC_MPICH_STATE_INIT;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.reduce_local.inbuf = inbuf;
    vtxp->nbargs.reduce_local.inoutbuf = inoutbuf;
    vtxp->nbargs.reduce_local.count = count;
    vtxp->nbargs.reduce_local.dt = datatype;
    vtxp->nbargs.reduce_local.op = operation;
    vtxp->nbargs.reduce_local.flags = flags;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [reduce_local]\n", vtx_id));
    return vtx_id;
}

/* Transport function to do a blocking data copy */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_dtcopy(void *tobuf,
                                  int tocount,
                                  MPIC_MPICH_dt_t totype,
                                  const void *frombuf, int fromcount, MPIC_MPICH_dt_t fromtype)
{
    return MPIR_Localcopy(frombuf,      /* yes, parameters are reversed        */
                          fromcount,    /* MPICH forgot what memcpy looks like */
                          fromtype, tobuf, tocount, totype);
}

/* Transport function to schedule a data copy */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_dtcopy_nb(void *tobuf,
                                     int tocount,
                                     MPIC_MPICH_dt_t totype,
                                     const void *frombuf,
                                     int fromcount,
                                     MPIC_MPICH_dt_t fromtype,
                                     MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);

    /* assign a new vertex */
    vtxp->kind = MPIC_MPICH_KIND_DTCOPY;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.dtcopy.tobuf = tobuf;
    vtxp->nbargs.dtcopy.tocount = tocount;
    vtxp->nbargs.dtcopy.totype = totype;
    vtxp->nbargs.dtcopy.frombuf = frombuf;
    vtxp->nbargs.dtcopy.fromcount = fromcount;
    vtxp->nbargs.dtcopy.fromtype = fromtype;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [dt_copy]\n", vtx_id));
    return vtx_id;
}

/* Internal transport function to add a single element to MPIC_MPICH_ptr_array */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_add_elem_ptr_array(MPIC_MPICH_ptr_array * buf_array, void *buf)
{
    if (buf_array->used == buf_array->size) {   /* increase array size */
        int old_size = buf_array->size;
        void **old_array = buf_array->array;
        buf_array->size *= 2;
        /* allocate new array */
        buf_array->array = (void **) MPIC_MPICH_allocate_mem(sizeof(void *) * buf_array->size);
        /* copy from old array to new array */
        memcpy(buf_array->array, old_array, sizeof(void *) * old_size);
        /* free old array */
        MPIC_MPICH_free_mem(old_array);
    }
    /* add element to the array */
    buf_array->array[buf_array->used++] = buf;
}

/* Transport function to allocate memory required for schedule execution.
 *The memory address is recorded by the transport in the schedule
 * so that this memory can be freed when the schedule is destroyed
 */
MPL_STATIC_INLINE_PREFIX void *MPIC_MPICH_allocate_buffer(size_t size, MPIC_MPICH_sched_t * s)
{
    void *buf = MPIC_MPICH_allocate_mem(size);
    /* record memory allocation */
    MPIC_MPICH_add_elem_ptr_array(&s->buf_array, buf);
    return buf;
}

/* Transport function to schedule memory free'ing */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_free_mem_nb(void *ptr,
                                       MPIC_MPICH_sched_t * sched, int n_invtcs, int *invtcs)
{
    MPIC_MPICH_vtx_t *vtxp;
    int vtx_id = MPIC_MPICH_get_new_vtx(sched, &vtxp);

    /* assign a new vertex */
    vtxp->kind = MPIC_MPICH_KIND_FREE_MEM;
    MPIC_MPICH_init_vtx(sched, vtxp, vtx_id);
    MPIC_MPICH_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.free_mem.ptr = ptr;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"TSP(mpich) : sched [%d] [free_mem]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIC_MPICH_issue_vtx
/* Internal transport function to issue a vertex */
/* This is set to be static inline because compiler cannot build this function with MPL_STATIC_INLINE_PREFIX
 * complaining that this is a non-inlineable function */
static inline void MPIC_MPICH_issue_vtx(int vtxid, MPIC_MPICH_vtx_t * vtxp,
                                        MPIC_MPICH_sched_t * sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_MPICH_ISSUE_VTX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_MPICH_ISSUE_VTX);

    int i;
    /* Check if the vertex has not already been issued and its incoming dependencies have completed */
    if (vtxp->state == MPIC_MPICH_STATE_INIT && vtxp->num_unfinished_dependencies == 0) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Issuing vertex %d\n", vtxid));

        switch (vtxp->kind) {
        case MPIC_MPICH_KIND_SEND:{
                MPIR_Errflag_t errflag = MPIR_ERR_NONE;
                /* issue task */
                MPIC_Isend(vtxp->nbargs.sendrecv.buf,
                           vtxp->nbargs.sendrecv.count,
                           vtxp->nbargs.sendrecv.dt,
                           vtxp->nbargs.sendrecv.dest,
                           sched->tag,
                           vtxp->nbargs.sendrecv.comm->mpid_comm, &vtxp->mpid_req[0], &errflag);
                /* record vertex issue */
                MPIC_MPICH_record_vtx_issue(vtxp, sched);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (isend) issued, tag = %d\n", sched->tag));
            }
            break;

        case MPIC_MPICH_KIND_RECV:{
                /* issue task */
                MPIC_Irecv(vtxp->nbargs.sendrecv.buf,
                           vtxp->nbargs.sendrecv.count,
                           vtxp->nbargs.sendrecv.dt,
                           vtxp->nbargs.sendrecv.dest,
                           sched->tag, vtxp->nbargs.sendrecv.comm->mpid_comm, &vtxp->mpid_req[0]);
                /* record vertex issue */
                MPIC_MPICH_record_vtx_issue(vtxp, sched);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (irecv) issued\n"));
            }
            break;
        case MPIC_MPICH_KIND_MULTICAST:{
                MPIR_Errflag_t errflag = MPIR_ERR_NONE;
                /* issue task */
                for (i=0; i<vtxp->nbargs.multicast.num_destinations; i++)
                    MPIC_Isend(vtxp->nbargs.multicast.buf,
                               vtxp->nbargs.multicast.count,
                               vtxp->nbargs.multicast.dt,
                               vtxp->nbargs.multicast.destinations[i],
                               sched->tag,
                               vtxp->nbargs.multicast.comm->mpid_comm, &vtxp->nbargs.multicast.mpir_req[i], &errflag);
                /* record vertex issue */
                MPIC_MPICH_record_vtx_issue(vtxp, sched);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (multicast) issued, tag = %d\n", sched->tag));
            }
            break;
        case MPIC_MPICH_KIND_ADDREF_DT:
            MPIC_MPICH_addref_dt(vtxp->nbargs.addref_dt.dt, vtxp->nbargs.addref_dt.up);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (addref dt) complete\n"));

            /* record vertex completion */
            MPIC_MPICH_record_vtx_completion(vtxp, sched);
            break;

        case MPIC_MPICH_KIND_ADDREF_OP:
            MPIC_MPICH_addref_op(vtxp->nbargs.addref_op.op, vtxp->nbargs.addref_op.up);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (addref op) complete\n"));

            /* record vertex completion */
            MPIC_MPICH_record_vtx_completion(vtxp, sched);
            break;

        case MPIC_MPICH_KIND_DTCOPY:
            MPIC_MPICH_dtcopy(vtxp->nbargs.dtcopy.tobuf,
                              vtxp->nbargs.dtcopy.tocount,
                              vtxp->nbargs.dtcopy.totype,
                              vtxp->nbargs.dtcopy.frombuf,
                              vtxp->nbargs.dtcopy.fromcount, vtxp->nbargs.dtcopy.fromtype);

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (dtcopy) complete\n"));
            /* record vertex completion */
            MPIC_MPICH_record_vtx_completion(vtxp, sched);

            break;

        case MPIC_MPICH_KIND_FREE_MEM:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (freemem) complete\n"));
            /* We are not really free'ing memory here, it will be needed when schedule is reused
             * The memory is free'd when the schedule is destroyed */

            /* record vertex completion */
            MPIC_MPICH_record_vtx_completion(vtxp, sched);
            break;

        case MPIC_MPICH_KIND_NOOP:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (noop) complete\n"));
            MPIC_MPICH_record_vtx_completion(vtxp, sched);
            break;

        case MPIC_MPICH_KIND_FENCE:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (fence) complete\n"));
            MPIC_MPICH_record_vtx_completion(vtxp, sched);
            break;

        case MPIC_MPICH_KIND_WAIT:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (wait) complete\n"));
            MPIC_MPICH_record_vtx_completion(vtxp, sched);
            break;

        case MPIC_MPICH_KIND_RECV_REDUCE:{
                MPIC_Irecv(vtxp->nbargs.recv_reduce.inbuf,
                           vtxp->nbargs.recv_reduce.count,
                           vtxp->nbargs.recv_reduce.datatype,
                           vtxp->nbargs.recv_reduce.source,
                           sched->tag, vtxp->nbargs.recv_reduce.comm->mpid_comm,
                           &vtxp->mpid_req[0]);
                MPIR_Grequest_start_impl(MPIC_MPICH_queryfcn, NULL, NULL, &vtxp->nbargs.recv_reduce,
                                         &vtxp->mpid_req[1]);

                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (recv_reduce) issued\n"));

                /* record vertex issue */
                MPIC_MPICH_record_vtx_issue(vtxp, sched);
            }
            break;

        case MPIC_MPICH_KIND_REDUCE_LOCAL:
            if (vtxp->nbargs.reduce_local.flags == -1 ||
                vtxp->nbargs.reduce_local.flags & MPIC_FLAG_REDUCE_L) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Left reduction vtxp->nbargs.reduce_local.flags %ld \n",
                         vtxp->nbargs.reduce_local.flags));
                MPIR_Reduce_local_impl((const void *) vtxp->nbargs.reduce_local.inbuf,
                                       (void *) vtxp->nbargs.reduce_local.inoutbuf,
                                       vtxp->nbargs.reduce_local.count,
                                       vtxp->nbargs.reduce_local.dt, vtxp->nbargs.reduce_local.op);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (reduce local_L) complete\n"));
            } else {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Right reduction vtxp->nbargs.reduce_local.flags %ld \n",
                         vtxp->nbargs.reduce_local.flags));
                /* An extra data copy is required to do the R reduction */
                MPIR_Reduce_local_impl((const void *) vtxp->nbargs.reduce_local.inoutbuf,
                                       (void *) vtxp->nbargs.reduce_local.inbuf,
                                       vtxp->nbargs.reduce_local.count,
                                       vtxp->nbargs.reduce_local.dt, vtxp->nbargs.reduce_local.op);

                MPIR_Localcopy(vtxp->nbargs.reduce_local.inbuf,
                               vtxp->nbargs.reduce_local.count,
                               vtxp->nbargs.reduce_local.dt,
                               vtxp->nbargs.reduce_local.inoutbuf,
                               vtxp->nbargs.reduce_local.count, vtxp->nbargs.reduce_local.dt);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (reduce local_R) complete\n"));
            }

            /* record vertex completion */
            MPIC_MPICH_record_vtx_completion(vtxp, sched);
            break;
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_MPICH_ISSUE_VTX);
}

#undef FUNCNAME
#define FUNCNAME MPIC_MPICH_test
/* Transport function to make progress on the schedule */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_test(MPIC_MPICH_sched_t * sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_MPICH_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_MPICH_TEST);

    MPIC_MPICH_vtx_t *vtxp;
    int i;

    /* If issued list is empty, go over the vertices and issue ready vertices
     * Issued list should be empty only when the MPIC_MPICH_test function
     * is called for the first time on the schedule */
    if (sched->issued_head == NULL) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"issued list is empty, issue ready vtcs\n"));
        if (sched->total > 0 && sched->num_completed != sched->total) {
            /* Go over all the vertices and issue ready vertices */
            for (i = 0; i < sched->total; i++)
                MPIC_MPICH_issue_vtx(i, &sched->vtcs[i], sched);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"completed traversal of vtcs, sched->total: %d, sched->num_completed: %d\n",
                     sched->total, sched->num_completed));
            return 0;
        } else
            return 1;   /* schedule completed */
    }

    if (sched->total == sched->num_completed) { /* if done, return 1 */
        return 1;
    }

    /* iterate over the issued vertex list, starting at the head */
    sched->vtx_iter = sched->issued_head;
    /* reset the issued vertex list, the list will be created again */
    sched->issued_head = NULL;

    /* Check for issued vertices that have been completed */
    while (sched->vtx_iter != NULL) {
        vtxp = sched->vtx_iter;
        /* vtx_iter is updated to point to the next issued vertex
         * that will be looked at after the current vertex */
        sched->vtx_iter = sched->vtx_iter->next_issued;

        if (vtxp->state == MPIC_MPICH_STATE_ISSUED) {

            MPI_Status status;
            MPIR_Request *mpid_req0 = vtxp->mpid_req[0];
            MPIR_Request *mpid_req1 = vtxp->mpid_req[1];

            /* If recv_reduce (mpid_req is not NULL), then make progress on it */
            if (mpid_req1) {
                (mpid_req1->u.ureq.greq_fns->query_fn)
                    (mpid_req1->u.ureq.greq_fns->grequest_extra_state, &status);
            }

            switch (vtxp->kind) {
            case MPIC_MPICH_KIND_SEND:
            case MPIC_MPICH_KIND_RECV:
                if (mpid_req0 && MPIR_Request_is_complete(mpid_req0)) {
                    MPIR_Request_free(mpid_req0);
                    vtxp->mpid_req[0] = NULL;
#ifdef MPIC_DEBUG
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (kind=%d) complete\n", vtxp->kind));
                    if (vtxp->nbargs.sendrecv.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"data sent/recvd: %d\n", *(int *) (vtxp->nbargs.sendrecv.buf)));
#endif
                    /* record vertex completion */
                    MPIC_MPICH_record_vtx_completion(vtxp, sched);
                } else
                    MPIC_MPICH_record_vtx_issue(vtxp, sched);   /* record it again as issued */
                break;
            case MPIC_MPICH_KIND_MULTICAST:
                for (i=vtxp->nbargs.multicast.last_complete+1; i<vtxp->nbargs.multicast.num_destinations; i++) {
                    if(MPIR_Request_is_complete(vtxp->nbargs.multicast.mpir_req[i])){
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport multicast task %d complete\n", i));
                        MPIR_Request_free(vtxp->nbargs.multicast.mpir_req[i]);
                        vtxp->nbargs.multicast.mpir_req[i] = NULL;
                        vtxp->nbargs.multicast.last_complete = i;
                    }
                    else /* we are only checking in sequence, hence break out at the first incomplete send */
                        break;
                }
                /* if all the sends have completed, mark this vertex as complete */
                if (i==vtxp->nbargs.multicast.num_destinations)
                    MPIC_MPICH_record_vtx_completion(vtxp, sched);
                else
                    MPIC_MPICH_record_vtx_issue(vtxp, sched);
                break;
            case MPIC_MPICH_KIND_RECV_REDUCE:
                /* check for recv completion */
                if (mpid_req0 && MPIR_Request_is_complete(mpid_req0)) {
                    MPIR_Request_free(mpid_req0);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"recv in recv_reduce completed\n"));
                    vtxp->mpid_req[0] = NULL;
                }

                /* check for reduce completion */
                if (mpid_req1 && MPIR_Request_is_complete(mpid_req1)) {
                    MPIR_Request_free(mpid_req1);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"reduce in recv_reduce also completed\n"));
                    vtxp->mpid_req[1] = NULL;

                    /* recv_reduce is now complete because reduce can complete
                     * only if the corresponding recv is complete */
#ifdef MPIC_DEBUG
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (kind=%d) complete\n", vtxp->kind));
                    if (vtxp->nbargs.sendrecv.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"data sent/recvd: %d\n", *(int *) (vtxp->nbargs.sendrecv.buf)));
#endif
                    /* record vertex completion */
                    MPIC_MPICH_record_vtx_completion(vtxp, sched);
                } else
                    MPIC_MPICH_record_vtx_issue(vtxp, sched);   /* record it again as issued */

                break;

            default:
                break;
            }
        }
    }
    /* set the tail of the issued vertex linked lsit */
    sched->last_issued->next_issued = NULL;

#ifdef MPIC_DEBUG
    if (sched->num_completed == sched->total) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"  --> MPICH transport (test) complete:  sched->total=%d\n", sched->total));
    }
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_MPICH_TEST);

    /* return 1 if complete, else return 0 */
    return sched->num_completed == sched->total;
}

/* Internal transport function to free any memory allocated for execution of this schedule */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_free_buffers(MPIC_MPICH_sched_t * sched)
{
    int i;
    for (i = 0; i < sched->total; i++) {
        /* free the temporary memory allocated by recv_reduce call */
        if (sched->vtcs[i].kind == MPIC_MPICH_KIND_RECV_REDUCE) {
            MPIC_MPICH_free_mem(sched->vtcs[i].nbargs.recv_reduce.inbuf);
            sched->vtcs[i].nbargs.recv_reduce.inbuf = NULL;
        }

        if (sched->vtcs[i].kind == MPIC_MPICH_KIND_FREE_MEM) {
            MPIC_MPICH_free_mem(sched->vtcs[i].nbargs.free_mem.ptr);
            sched->vtcs[i].nbargs.free_mem.ptr = NULL;
        }

        if(sched->vtcs[i].kind == MPIC_MPICH_KIND_MULTICAST){
            MPIC_MPICH_free_mem(sched->vtcs[i].nbargs.multicast.mpir_req);
            MPIC_MPICH_free_mem(sched->vtcs[i].nbargs.multicast.destinations);
        }
    }
    /* free temporary buffers allocated using MPIC_MPICH_allocate_buffer call */
    for (i = 0; i < sched->buf_array.used; i++) {
        MPIC_MPICH_free_mem(sched->buf_array.array[i]);
    }
    if(sched->buf_array.size!=0)
        MPIC_MPICH_free_mem(sched->buf_array.array);

    /* free each vtx and then the list of vtcs */
    for (i = 0; i < sched->total; i++) {
        /* up to sched->total because we init vertices only when we need them */
        MPIC_MPICH_free_vtx(&sched->vtcs[i]);
    }
    /* free the list of vertices */
    MPIC_MPICH_free_mem(sched->vtcs);
}

/* Internal transport function to destroy all memory associated with a schedule */
MPL_STATIC_INLINE_PREFIX void MPIC_MPICH_sched_destroy_fn(MPIC_MPICH_sched_t * sched)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"In MPIC_MPICH_sched_destroy_fn for schedule: %p\n", sched));
    MPIC_MPICH_free_buffers(sched);
    MPL_free(sched);
}

/* Transport function to initialize a transport communicator */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_comm_init(MPIC_MPICH_comm_t * comm, void *base)
{
    MPIR_Comm *mpi_comm = container_of(base, MPIR_Comm, coll);
    comm->mpid_comm = mpi_comm;
    comm->sched_db = NULL;
    return 0;
}

/* Transport function to cleanup a transport communicator */
MPL_STATIC_INLINE_PREFIX int MPIC_MPICH_comm_cleanup(MPIC_MPICH_comm_t * comm)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"In MPIC_MPICH_comm_cleanup for comm: %p\n", comm));
    /* free up schedule database */
    MPIC_delete_sched_table(comm->sched_db, (MPIC_sched_free_fn) MPIC_MPICH_sched_destroy_fn);
    comm->mpid_comm = NULL;
    return 0;
}

#endif
