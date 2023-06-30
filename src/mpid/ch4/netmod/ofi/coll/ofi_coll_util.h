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
    int context_offset = MPIR_CONTEXT_COLL_OFFSET;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    /* Build a knomial tree */
    my_tree->children = NULL;
    mpi_errno =
        MPIR_Treealgo_tree_create(myrank, nranks, tree_type, branching_factor, root, my_tree);
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
                                                   MPIDI_OFI_comm_to_phys(comm_ptr, *p, 0, 0),
                                                   MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                   rtr_tag, comm_ptr, *p, 0, *rcv_cntr, *rcv_cntr,
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
                                                                                0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, buffer,
                                                         count * data_sz, tag, comm_ptr, parent, 0,
                                                         *rcv_cntr, *rcv_cntr, false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
        i++;
    }

    index = i;

    if (!is_root) {     /* Non-root nodes send RTR to parents */
        uint64_t match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, comm_ptr->rank, rtr_tag,
                                   0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0), match_bits), 0,
                             tinject);
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
                                                         MPIDI_OFI_comm_to_phys(comm_ptr, *p, 0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_SEND, buffer,
                                                         count * data_sz, tag, comm_ptr, *p,
                                                         threshold, *rcv_cntr, *snd_cntr, false);
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
    int context_offset = MPIR_CONTEXT_COLL_OFFSET;

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
                                                                                0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                         rtr_tag, comm_ptr, first_child + j, 0,
                                                         *rcv_cntr, *rcv_cntr, false);
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
                                                         MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0,
                                                                                0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, buffer,
                                                         count * data_sz, tag, comm_ptr, parent, 0,
                                                         *rcv_cntr, *rcv_cntr, false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
        i++;
    }

    index = i;

    if (!is_root) {     /* Non-root nodes send RTR to parents */
        s_match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, comm_ptr->rank, rtr_tag,
                                   0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0), s_match_bits), 0,
                             tinject);
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
                                                                                0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_SEND, buffer,
                                                         count * data_sz, tag, comm_ptr,
                                                         first_child + k, threshold, *rcv_cntr,
                                                         *snd_cntr, false);

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
    int context_offset = MPIR_CONTEXT_COLL_OFFSET;

    int i = 0, j = 0, k = 0;
    int parent = -1, *p;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(datatype, 1, data_sz);

    /* Build a knomial tree */
    my_tree->children = NULL;
    mpi_errno =
        MPIR_Treealgo_tree_create(myrank, nranks, tree_type, branching_factor, root, my_tree);
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
                                                         MPIDI_OFI_comm_to_phys(comm_ptr, *p, 0, 0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                         rtr_tag, comm_ptr, *p, 0, *rcv_cntr,
                                                         *rcv_cntr, false);
        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }

        j++;
    }

    i = i + j;

    if (!is_root) {     /* Non-root nodes send RTR to parents */
        s_match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, comm_ptr->rank, rtr_tag,
                                   0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0), s_match_bits), 0,
                             tinject);
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
                                                      MPIDI_OFI_comm_to_phys(comm_ptr, *p, 0, 0),
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
    int context_offset = MPIR_CONTEXT_COLL_OFFSET;

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
                                                                                0),
                                                         MPIDI_OFI_TRIGGERED_TAGGED_RECV, NULL, 0,
                                                         rtr_tag, comm_ptr, first_child + j, 0,
                                                         *rcv_cntr, *rcv_cntr, false);

        if (mpi_errno) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofid_issue_trigger");
        }
    }

    i = i + j;

    if (!is_root) {     /* Non-root nodes send RTR to parents; this is needed to avoid unexpected messages */
        s_match_bits =
            MPIDI_OFI_init_sendtag(comm_ptr->context_id + context_offset, comm_ptr->rank, rtr_tag,
                                   0);
        MPIDI_OFI_CALL_RETRY(fi_tinject
                             (MPIDI_OFI_global.ctx[0].tx, NULL, 0,
                              MPIDI_OFI_comm_to_phys(comm_ptr, parent, 0, 0), s_match_bits), 0,
                             tinject);
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
                                          MPIDI_OFI_comm_to_phys(comm_ptr, first_child + k, 0, 0),
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

#endif /* OFI_COLL_UTIL_H_INCLUDED */
