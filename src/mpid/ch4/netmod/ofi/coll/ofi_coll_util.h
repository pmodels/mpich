/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_COLL_UTIL_H_INCLUDED
#define OFI_COLL_UTIL_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"

/*
 * This function performs a knomial tree based Ibcast using two-sided communication.
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * tree_type - type of tree (knomial_1 or knomial_2)
 * branching_factor - branching factor of the tree
 * num_children - number of children for this rank
 * snd_cntr - send counter
 * rcv_cntr - receive counter
 * works - array of deferred works
 * my_tree - tree to be generated
 * myrank - rank ID of this rank
 * nranks - number of ranks
 * num_works - Number of deferred works
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Ibcast_knomial_triggered_tagged(void *buffer, int count,
                                                                       MPI_Datatype datatype,
                                                                       int root,
                                                                       MPIR_Comm * comm_ptr,
                                                                       int tree_type,
                                                                       int branching_factor,
                                                                       int *num_children,
                                                                       struct fid_cntr **snd_cntr,
                                                                       struct fid_cntr **rcv_cntr,
                                                                       struct fi_deferred_work
                                                                       **works,
                                                                       MPIR_Treealgo_tree_t *
                                                                       my_tree, int myrank,
                                                                       int nranks, int *num_works)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    struct fi_cntr_attr cntr_attr = {
        .events = FI_CNTR_EVENTS_COMP,
    };
    int threshold, tag, rtr_tag, ret;
    int *p;
    int i = 0, j = 0, k = 0, index = 0, is_root = 0, parent = -1;
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    /* Build a knomial tree */
    my_tree->children = NULL;
    MPIR_Treealgo_params_t tree_params = {
        .rank = myrank,
        .nranks = nranks,
        .k = branching_factor,
        .tree_type = tree_type,
        .root = 0
    };
    mpi_errno = MPIR_Treealgo_tree_create(comm_ptr, &tree_params, my_tree);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    *num_children = my_tree->num_children;
    parent = my_tree->parent;

    if (myrank == root)
        is_root = 1;

    /* Initialize counters for triggered ops */
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, snd_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, rcv_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    /* Allocate deferred works as needed by the current rank */
    *works =
        (struct fi_deferred_work *) MPL_malloc(2 * (*num_children + 1) *
                                               sizeof(struct fi_deferred_work), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDSTMT(*works == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &rtr_tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Post recv for RTR from children */
    for (p = (int *) utarray_front(my_tree->children); p != NULL;
         p = (int *) utarray_next(my_tree->children, p)) {
        ret = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                   MPIDI_OFI_global.ctx[0].rx, &((*works)[i + j]),
                                                   MPIDI_OFI_comm_to_phys(comm_ptr, *p, 0, 0, 0),
                                                   MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                   rtr_tag, comm_ptr, 0, *rcv_cntr, *rcv_cntr,
                                                   false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
        j++;
    }

    i = i + j;
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!is_root) {     /* non-root nodes post recv for data from parents */
        mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].rx, &((*works)[i]),
                                                         MPIDI_OFI_comm_to_phys(comm_ptr, parent,
                                                                                0, 0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, buffer,
                                                         count * data_sz, tag, comm_ptr, 0,
                                                         *rcv_cntr, *rcv_cntr, false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
        i++;
    }

    index = i;

    if (!is_root) {     /* Non-root nodes send RTR to parents */
        uint64_t match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, rtr_tag, 0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                              match_bits), 0, tinject, FALSE);
    }

    if (is_root) {
        threshold = *num_children;

    } else {
        threshold = 1 + *num_children;
    }

    /* Root and intermediate nodes send data to children */
    for (p = (int *) utarray_front(my_tree->children); p != NULL;
         p = (int *) utarray_next(my_tree->children, p)) {
        mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].tx,
                                                         &((*works)[index + k]),
                                                         MPIDI_OFI_comm_to_phys(comm_ptr, *p, 0,
                                                                                0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_SEND, buffer,
                                                         count * data_sz, tag, comm_ptr, threshold,
                                                         *rcv_cntr, *snd_cntr, false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
        k++;
    }

    *num_works = index + k;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * This function performs a kary tree based Ibcast using two-sided communication.
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * tree_type - type of tree (kary)
 * branching_factor - branching factor of the tree
 * is_leaf - indicates if this rank is a leaf rank
 * children - number of children for this rank
 * snd_cntr - send counter
 * rcv_cntr - receive counter
 * works - array of deferred works
 * myrank - rank ID of this rank
 * nranks - number of ranks
 * num_works - Number of deferred works
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Ibcast_kary_triggered_tagged(void *buffer, int count,
                                                                    MPI_Datatype datatype, int root,
                                                                    MPIR_Comm * comm_ptr,
                                                                    int branching_factor,
                                                                    int *is_leaf, int *children,
                                                                    struct fid_cntr **snd_cntr,
                                                                    struct fid_cntr **rcv_cntr,
                                                                    struct fi_deferred_work **works,
                                                                    int myrank, int nranks,
                                                                    int *num_works)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    struct fi_cntr_attr cntr_attr = {
        .events = FI_CNTR_EVENTS_COMP,
    };
    int threshold;
    int is_root = 0, ret, tag, rtr_tag;
    uint64_t s_match_bits;
    int i = 0, j = 0, k = 0, index = 0;
    int intermediate = 0, first_child, parent = -1, leaf = 0, num_children = 0;
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    /* Build a kary tree */
    MPIDI_OFI_build_kary_tree(myrank, nranks, root, branching_factor, &first_child,
                              &parent, &num_children, &is_root, &leaf, &intermediate);

    /* Initialize counters for triggered ops */
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, snd_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, rcv_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    /* Allocate deferred works as needed by the current rank */
    *works =
        (struct fi_deferred_work *) MPL_malloc(2 * (num_children + 1) *
                                               sizeof(struct fi_deferred_work), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDSTMT(*works == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");


    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &rtr_tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Post recv for RTR from children */
    for (j = 0; j < num_children; j++) {
        mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].rx,
                                                         &((*works)[i + j]),
                                                         MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                                first_child + j, 0,
                                                                                0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                         rtr_tag, comm_ptr, 0, *rcv_cntr, *rcv_cntr,
                                                         false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
    }

    i = i + j;
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!is_root) {     /* Non-root nodes post recv for data */
        mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].rx, &((*works)[i]),
                                                         MPIDI_OFI_comm_to_phys(comm_ptr, parent,
                                                                                0, 0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, buffer,
                                                         count * data_sz, tag, comm_ptr, 0,
                                                         *rcv_cntr, *rcv_cntr, false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
        i++;
    }

    index = i;

    if (!is_root) {     /* Non-root nodes send RTR to parents */
        s_match_bits = MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, rtr_tag, 0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                              s_match_bits), 0, tinject, FALSE);
    }

    if (is_root) {
        threshold = num_children;

    } else {
        threshold = 1 + num_children;
    }

    /* Root and intremediate nodes send data to children */
    for (k = 0; k < num_children; k++) {
        mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].tx,
                                                         &((*works)[index + k]),
                                                         MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                                first_child + k, 0,
                                                                                0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_SEND, buffer,
                                                         count * data_sz, tag, comm_ptr, threshold,
                                                         *rcv_cntr, *snd_cntr, false);

        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
    }

    *num_works = index + k;
    *is_leaf = leaf;
    *children = num_children;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * This function performs a knomial tree based Ibcast using one-sided communication.
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * tree_type - type of tree (kary, knomial_1 or knomial_2)
 * branching_factor - branching factor of the tree
 * num_children - number of children for this rank
 * snd_cntr - send counter
 * rcv_cntr - receive counter
 * r_mr - remote key for RMA
 * works - array of deferred works
 * my_tree - tree to be generated
 * myrank - rank ID of this rank
 * nranks - number of ranks
 * num_works - Number of deferred works
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Ibcast_knomial_triggered_rma(void *buffer, int count,
                                                                    MPI_Datatype datatype, int root,
                                                                    MPIR_Comm * comm_ptr,
                                                                    int tree_type,
                                                                    int branching_factor,
                                                                    int *num_children,
                                                                    struct fid_cntr **snd_cntr,
                                                                    struct fid_cntr **rcv_cntr,
                                                                    struct fid_mr **r_mr,
                                                                    struct fi_deferred_work **works,
                                                                    MPIR_Treealgo_tree_t * my_tree,
                                                                    int myrank, int nranks,
                                                                    int *num_works)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    struct fi_cntr_attr cntr_attr = {
        .events = FI_CNTR_EVENTS_COMP,
    };
    int threshold;
    int is_root = 0, ret;
    int rtr_tag;
    uint64_t s_match_bits;
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    int i = 0, j = 0, k = 0;
    int parent = -1, *p;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    /* Build a knomial tree */
    my_tree->children = NULL;
    MPIR_Treealgo_params_t tree_params = {
        .rank = myrank,
        .nranks = nranks,
        .k = branching_factor,
        .tree_type = tree_type,
        .root = 0
    };
    mpi_errno = MPIR_Treealgo_tree_create(comm_ptr, &tree_params, my_tree);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    *num_children = my_tree->num_children;
    parent = my_tree->parent;


    if (myrank == root)
        is_root = 1;

    /* Initialize counters for triggered ops */
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, snd_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, rcv_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    /* Register memory for rma */
    MPIDI_OFI_CALL(fi_mr_reg
                   (MPIDI_OFI_global.ctx[0].domain, buffer, count * data_sz,
                    FI_REMOTE_WRITE | FI_REMOTE_READ, 0ULL, MPIDI_OFI_COMM(comm_ptr).conn_id,
                    FI_RMA_EVENT, r_mr, NULL), mr_reg);
    if (MPIDI_OFI_global.prov_use[0]->domain_attr->mr_mode == FI_MR_ENDPOINT) {
        /* Bind the memory region to the endpoint */
        MPIDI_OFI_CALL(fi_mr_bind(*r_mr, &MPIDI_OFI_global.ctx[0].ep->fid, 0ULL), mr_bind);
    }

    /* Bind counters with the memory region so that when data is received in this region, the counter
     *      * gets incremented */
    MPIDI_OFI_CALL(fi_mr_bind(*r_mr, &(*rcv_cntr)->fid, FI_REMOTE_WRITE | FI_REMOTE_READ), mr_bind);
    fi_mr_enable(*r_mr);

    /* Allocate deferred works */
    *works =
        (struct fi_deferred_work *) MPL_malloc((2 * (*num_children + 1)) *
                                               sizeof(struct fi_deferred_work), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP1(*works == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "Triggered bcast deferred work alloc");

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &rtr_tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Post recv for RTR from children; this is needed to avoid unexpected messages */
    for (p = (int *) utarray_front(my_tree->children); p != NULL;
         p = (int *) utarray_next(my_tree->children, p)) {
        mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].rx,
                                                         &((*works)[i + j]),
                                                         MPIDI_OFI_comm_to_phys(comm_ptr, *p, 0,
                                                                                0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                         rtr_tag, comm_ptr, 0, *rcv_cntr, *rcv_cntr,
                                                         false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }

        j++;
    }

    i = i + j;

    if (!is_root) {     /* Non-root nodes send RTR to parents */
        s_match_bits = MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, rtr_tag, 0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                              s_match_bits), 0, tinject, FALSE);
    }

    if (is_root) {
        threshold = *num_children;
    } else {
        threshold = 1 + *num_children;
    }

    /* Root and intermediate nodes send data to children */
    for (p = (int *) utarray_front(my_tree->children); p != NULL;
         p = (int *) utarray_next(my_tree->children, p)) {
        mpi_errno = MPIDI_OFI_prepare_rma_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                      MPIDI_OFI_global.ctx[0].tx, &((*works)[i++]),
                                                      MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                             *p, 0, 0, 0),
                                                      buffer, fi_mr_key(*r_mr),
                                                      count * data_sz, threshold, *rcv_cntr,
                                                      *snd_cntr, MPIDI_OFI_TRIGGERED_RMA_WRITE,
                                                      false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
    }

    *num_works = i + k;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * This function performs a kary tree based Ibcast using one-sided communication.
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * branching_factor - branching factor of the tree
 * is_leaf - indicates if this rank is a leaf rank
 * children - number of children for this rank
 * snd_cntr - send counter
 * rcv_cntr - receive counter
 * r_mr - remote key for RMA
 * works - array of deferred works
 * myrank - rank ID of this rank
 * nranks - number of ranks
 * num_works - Number of deferred works
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Ibcast_kary_triggered_rma(void *buffer, int count,
                                                                 MPI_Datatype datatype, int root,
                                                                 MPIR_Comm * comm_ptr,
                                                                 int branching_factor, int *is_leaf,
                                                                 int *children,
                                                                 struct fid_cntr **snd_cntr,
                                                                 struct fid_cntr **rcv_cntr,
                                                                 struct fid_mr **r_mr,
                                                                 struct fi_deferred_work **works,
                                                                 int myrank, int nranks,
                                                                 int *num_works)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    struct fi_cntr_attr cntr_attr = {
        .events = FI_CNTR_EVENTS_COMP,
    };
    int threshold;
    int is_root = 0, ret;
    int rtr_tag;
    uint64_t s_match_bits;
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    int i = 0, j = 0, k = 0;
    int intermediate = 0, first_child, parent = -1, leaf = 0;
    int num_children = 0;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    /* Build a kary tree */
    MPIDI_OFI_build_kary_tree(myrank, nranks, root, branching_factor, &first_child,
                              &parent, &num_children, &is_root, &leaf, &intermediate);

    /* Initialize counters for triggered ops */
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, snd_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, rcv_cntr, NULL),
                          ret);
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cntr_open");

    /* Register memory for rma */
    MPIDI_OFI_CALL(fi_mr_reg
                   (MPIDI_OFI_global.ctx[0].domain, buffer, count * data_sz,
                    FI_REMOTE_WRITE | FI_REMOTE_READ, 0ULL, MPIDI_OFI_COMM(comm_ptr).conn_id,
                    FI_RMA_EVENT, r_mr, NULL), mr_reg);
    if (MPIDI_OFI_global.prov_use[0]->domain_attr->mr_mode == FI_MR_ENDPOINT) {
        /* Bind the memory region to the endpoint */
        MPIDI_OFI_CALL(fi_mr_bind(*r_mr, &MPIDI_OFI_global.ctx[0].ep->fid, 0ULL), mr_bind);
    }

    /* Bind counters with the memory region so that when data is received in this region, the counter
     * gets incremented */
    MPIDI_OFI_CALL(fi_mr_bind(*r_mr, &(*rcv_cntr)->fid, FI_REMOTE_WRITE | FI_REMOTE_READ), mr_bind);
    fi_mr_enable(*r_mr);


    /* Allocate deferred works as needed by the current rank */
    *works =
        (struct fi_deferred_work *) MPL_malloc(2 * (num_children + 1) *
                                               sizeof(struct fi_deferred_work), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDSTMT(*works == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &rtr_tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Post recv for RTR from children */
    for (j = 0; j < num_children; j++) {
        mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                         MPIDI_OFI_global.ctx[0].rx,
                                                         &((*works)[i + j]),
                                                         MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                                first_child + j, 0,
                                                                                0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                         rtr_tag, comm_ptr, 0, *rcv_cntr, *rcv_cntr,
                                                         false);

        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
    }

    i = i + j;

    if (!is_root) {     /* Non-root nodes send RTR to parents; this is needed to avoid unexpected messages */
        s_match_bits = MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, rtr_tag, 0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                              s_match_bits), 0, tinject, FALSE);
    }

    if (is_root) {
        threshold = num_children;

    } else {
        threshold = 1 + num_children;
    }

    /* Root and intremediate nodes send data to children */
    for (k = 0; k < num_children; k++) {
        MPIDI_OFI_prepare_rma_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                          MPIDI_OFI_global.ctx[0].tx, &((*works)[i + k]),
                                          MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                 first_child + k, 0, 0, 0),
                                          buffer, fi_mr_key(*r_mr),
                                          count * data_sz, threshold, *rcv_cntr, *snd_cntr,
                                          MPIDI_OFI_TRIGGERED_RMA_WRITE, false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
    }

    *num_works = i + k;
    *is_leaf = leaf;
    *children = num_children;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * This function performs a kary tree based pipelined bcast for large messages using one-sided communication.
 * This is a non-blocking function.
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * branching_factor - branching factor of the tree
 * is_leaf - indicates if this rank is a leaf rank
 * children - number of children for this rank
 * snd_cntr - send counter
 * rcv_cntr - receive counter
 * r_mr - array of remote keys for RMA
 * works - array of deferred works
 * myrank - rank ID of this rank
 * nranks - number of ranks
 * num_works - number of deferred works
 * num_chunks - umber of chunks for the pipelined Bcast
 * chunk_size - chunk size for pipelining
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Ibcast_kary_triggered_pipelined(void *buffer, int count,
                                                                       MPI_Datatype datatype,
                                                                       int root,
                                                                       MPIR_Comm * comm_ptr,
                                                                       int branching_factor,
                                                                       int *is_leaf, int *children,
                                                                       struct fid_cntr **snd_cntr,
                                                                       struct fid_cntr ***rcv_cntr,
                                                                       struct fid_mr ***r_mr,
                                                                       struct fi_deferred_work
                                                                       **works, int myrank,
                                                                       int nranks, int *num_works,
                                                                       int *num_chunks,
                                                                       int chunk_size)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    struct fi_cntr_attr cntr_attr = {
        .events = FI_CNTR_EVENTS_COMP,
    };
    int threshold, ret;
    int is_root = 0, i = 0, j = 0, k = 0, index = 0;
    int intermediate = 0, first_child, parent = -1, leaf = 0, num_children = 0;
    int active_chunks = 0, posted_chunks = 0, trig_index = 0;
    int rtr_tag;
    uint64_t s_match_bits = 0;
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    *num_chunks = ceil(data_sz * count / chunk_size);

    /* Build a kary tree */
    MPIDI_OFI_build_kary_tree(myrank, nranks, root, branching_factor, &first_child,
                              &parent, &num_children, &is_root, &leaf, &intermediate);

    active_chunks = (*num_chunks < MPIDI_OFI_TRIGGERED_WINDOW) ? *num_chunks : MPIDI_OFI_TRIGGERED_WINDOW;      /* Number of active chunks <= the window size N */

    /* Initialize counters for triggered ops */
    MPIDI_OFI_CALL_RETURN(fi_cntr_open(MPIDI_OFI_global.ctx[0].domain, &cntr_attr, snd_cntr, NULL),
                          ret);
    if (ret < 0) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "fi_cntr_open (%s)\n", fi_strerror(ret)));
        mpi_errno = MPIDI_OFI_ENAVAIL;
        goto fn_fail;
    }

    /* register memory for the RMA keys */
    *r_mr = (struct fid_mr **) MPL_malloc(*num_chunks * sizeof(struct fid_mr *), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP1(*r_mr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "Triggered bcast RMA key alloc");

    for (i = 0; i < *num_chunks; i++) {

        MPIDI_OFI_CALL(fi_mr_reg
                       (MPIDI_OFI_global.ctx[0].domain, (unsigned char *) buffer + i * chunk_size,
                        chunk_size, FI_REMOTE_WRITE, 0ULL, i + 1, FI_RMA_EVENT, &((*r_mr)[i]),
                        NULL), mr_reg);
        if (MPIDI_OFI_global.prov_use[0]->domain_attr->mr_mode == FI_MR_ENDPOINT) {
            /* Bind the memory region to the endpoint */
            MPIDI_OFI_CALL(fi_mr_bind(((*r_mr)[i]), &MPIDI_OFI_global.ctx[0].ep->fid, 0ULL),
                           mr_bind);
        }

        assert(ret == 0);
    }

    *rcv_cntr =
        (struct fid_cntr **) MPL_malloc((*num_chunks + 1) * sizeof(struct fid_cntr *),
                                        MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP1(*rcv_cntr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "Triggered bcast array of recv counters alloc");

    /* Bind the receive buffer of each chunk with the corresponding receive counter */
    for (i = 0; i < *num_chunks; i++) {
        MPIDI_OFI_CALL_RETURN(fi_cntr_open
                              (MPIDI_OFI_global.ctx[0].domain, &cntr_attr, &((*rcv_cntr)[i]), NULL),
                              ret);
        if (ret < 0) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "fi_cntr_open (%s)\n", fi_strerror(ret)));
            mpi_errno = MPIDI_OFI_ENAVAIL;
            goto fn_fail;
        }

        MPIDI_OFI_CALL(fi_mr_bind((*r_mr)[i], &((*rcv_cntr)[i])->fid, FI_REMOTE_WRITE), mr_bind);
        fi_mr_enable((*r_mr)[i]);
    }

    (*rcv_cntr)[i] = *snd_cntr;

    /* Allocate deferred works */
    *works =
        (struct fi_deferred_work *) MPL_malloc((3 * (*num_chunks) * num_children + *num_chunks) *
                                               sizeof(struct fi_deferred_work), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP1(*works == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "Triggered bcast deferred work alloc");

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &rtr_tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Post recv for RTR from children */
    if (!leaf) {
        for (i = posted_chunks; i < posted_chunks + active_chunks; i++) {
            for (j = 0; j < num_children; j++) {
                mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                                 MPIDI_OFI_global.ctx[0].rx,
                                                                 &((*works)[index++]),
                                                                 MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                                        first_child
                                                                                        + j, 0, 0,
                                                                                        0),
                                                                 MPIDI_OFI_TRIGGERED_TAGGED_RECV,
                                                                 NULL, 0, rtr_tag, comm_ptr, 0,
                                                                 (*rcv_cntr)[i], (*rcv_cntr)[i],
                                                                 false);
                if (mpi_errno) {
                    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
                }
            }
        }
    }

    if (!is_root) {     /* Non-root nodes send RTR to parents */
        s_match_bits = MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, rtr_tag, 0);
        for (i = posted_chunks; i < (posted_chunks + active_chunks); i++) {
            MPIDI_OFI_CALL_RETRY(fi_tinject
                                 (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                                  MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                                  s_match_bits), 0, tinject, FALSE);
        }
    }

    while (posted_chunks < *num_chunks) {
        if (is_root) {
            threshold = num_children;

        } else {
            threshold = 1 + num_children;
        }

        /* Root and intremediate nodes send data to children */
        if (!leaf) {
            for (i = posted_chunks; i < (posted_chunks + active_chunks); i++) {
                for (k = 0; k < num_children; k++) {
                    mpi_errno = MPIDI_OFI_prepare_rma_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                                  MPIDI_OFI_global.ctx[0].tx,
                                                                  &((*works)[index++]),
                                                                  MPIDI_OFI_comm_to_phys(comm_ptr,
                                                                                         first_child
                                                                                         + k, 0,
                                                                                         0, 0),
                                                                  (unsigned char *) buffer +
                                                                  i * chunk_size,
                                                                  fi_mr_key((*r_mr)[i]), chunk_size,
                                                                  threshold, (*rcv_cntr)[i],
                                                                  *snd_cntr,
                                                                  MPIDI_OFI_TRIGGERED_RMA_WRITE,
                                                                  false);
                    if (mpi_errno) {
                        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
                    }
                }
            }
        }

        if ((posted_chunks > 0) && (posted_chunks % MPIDI_OFI_TRIGGERED_WINDOW == 0)) {
            trig_index = posted_chunks - MPIDI_OFI_TRIGGERED_WINDOW;
            if (is_root) {
                threshold = num_children;
            } else if (intermediate) {
                threshold = num_children + 1;
            }
        } else if (posted_chunks == 0) {
            threshold = 0;
            trig_index = 0;
        }

        threshold = 0;
        posted_chunks += active_chunks;
        active_chunks =
            (*num_chunks - posted_chunks) >
            MPIDI_OFI_TRIGGERED_WINDOW ? MPIDI_OFI_TRIGGERED_WINDOW : (*num_chunks - posted_chunks);

        if (!leaf) {    /* Send RTR for the next window of chunks */
            for (i = posted_chunks; i < posted_chunks + active_chunks; i++) {
                for (j = 0; j < num_children; j++) {
                    mpi_errno = MPIDI_OFI_prepare_tagged_control_cmd(MPIDI_OFI_global.ctx[0].domain,
                                                                     MPIDI_OFI_global.ctx[0].rx,
                                                                     &((*works)[index++]),
                                                                     MPIDI_OFI_comm_to_phys
                                                                     (comm_ptr, first_child + j, 0,
                                                                      0, 0),
                                                                     MPIDI_OFI_TRIGGERED_TAGGED_RECV,
                                                                     NULL, 0, rtr_tag, comm_ptr,
                                                                     threshold,
                                                                     (*rcv_cntr)[trig_index],
                                                                     (*rcv_cntr)[i], false);
                    if (mpi_errno) {
                        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
                    }
                }
            }
        }

        if (!is_root) {
            for (i = posted_chunks; i < (posted_chunks + active_chunks); i++) {
                MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                                                MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0, 0),
                                                s_match_bits), 0, tinject, FALSE);
            }
        }
    }

    *num_works = index;
    *is_leaf = leaf;
    *children = num_children;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_COLL_UTIL_H_INCLUDED */
