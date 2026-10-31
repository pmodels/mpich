/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef COLL_CSEL_H_INCLUDED
#define COLL_CSEL_H_INCLUDED

#include "csel_json.h"

/* "dispatch=<prefix>(<bits>)": continue in the subtree "<prefix>-<m>", m the restrictions (in hex)
 * among bits that hold. idx[] has one subtree per combination of the bits, resolved when the file
 * is loaded: the subtree of the combination, or of its largest subset that has one. */
typedef struct MPIR_Csel_dispatch {
    unsigned bits;
    int nbits;
    int pos[32];                /* bit positions of bits, low to high */
    int idx[];                  /* 1 << nbits subtree indices, by compressed mask */
} MPIR_Csel_dispatch_s;

typedef struct MPIR_Csel_node {
    MPIR_Csel_node_type_e type;
    union {
        struct {
            int idx;
        } call;
        MPIR_Csel_dispatch_s *dispatch;
        struct {
            MPIR_Csel_coll_type_e coll_type;
        } collective;
        struct {
            bool negate;
            int thresh;
        } condition;
        MPII_Csel_container_s *container;
    } u;
    struct MPIR_Csel_node *success;
    struct MPIR_Csel_node *failure;
} MPIR_Csel_node_s;

extern MPIR_Coll_algo_fn *MPIR_Coll_algo_table;
extern const int *const MPIR_Coll_cvar_table[];
extern const char **MPIR_Coll_type_names;
extern const char **MPIR_Coll_algo_names;
extern const char **MPIR_Csel_condition_names;
extern bool *MPIR_Csel_condition_has_thresh;
extern int *MPIR_Coll_algo_counters;

int MPIR_Csel_load_file(const char *json_file);
int MPIR_Csel_load_buf(const char *json_str);
int MPIR_Csel_free(void);
MPIR_Csel_node_s *MPIR_Csel_get_tree(const char *name);
MPII_Csel_container_s *MPIR_Csel_search(MPIR_Csel_node_s * csel, MPIR_Csel_coll_sig_s * coll_sig);
void MPIR_Csel_print_tree(MPIR_Csel_node_s * node, int level);
int MPIR_Coll_run_tree(MPIR_Csel_node_s * tree, MPIR_Csel_coll_sig_s * coll_sig);
int MPIR_Coll_run_auto(MPIR_Csel_coll_sig_s * coll_sig);

/* generated routines by gen_coll.py from coll_algorithms.txt */
int MPII_Csel_parse_container_params(const MPII_Csel_json_s * obj, MPII_Csel_container_s * cnt);
MPIR_Csel_node_s *MPII_Csel_run_condition(MPIR_Csel_node_s * node, MPIR_Csel_coll_sig_s * coll_sig);

MPL_STATIC_INLINE_PREFIX int MPIR_Csel_comm_size(MPIR_Csel_coll_sig_s * coll_sig)
{
    /* FIXME: update when we have intercomm algorithms that need select on comm_size */
    MPIR_Assert(coll_sig->comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    return coll_sig->comm_ptr->local_size;
}

/* the number of nodes the comm spans */
MPL_STATIC_INLINE_PREFIX int MPIR_Csel_num_nodes(MPIR_Csel_coll_sig_s * coll_sig)
{
    MPIR_Comm *comm = coll_sig->comm_ptr;
    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    MPIR_Assert(comm->attr & MPIR_COMM_ATTR__HIERARCHY);
    return comm->num_external;
}

/* ranks per node, averaged over the nodes (rounded down): the same on every rank, also when
 * nodes have different numbers of ranks */
MPL_STATIC_INLINE_PREFIX int MPIR_Csel_ppn(MPIR_Csel_coll_sig_s * coll_sig)
{
    MPIR_Comm *comm = coll_sig->comm_ptr;
    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    MPIR_Assert(comm->attr & MPIR_COMM_ATTR__HIERARCHY);
    return comm->local_size / comm->num_external;
}

#define COLL_TYPE_ALL_CASE(NAME) \
    case MPIR_CSEL_COLL_TYPE__INTRA_  ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTER_  ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTRA_I ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTER_I ## NAME

#define COLL_TYPE_INTRA_CASE(NAME) \
    case MPIR_CSEL_COLL_TYPE__INTRA_  ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTRA_I ## NAME

/* the mean of a recvcounts array, at least 1 when any count is nonzero */
MPL_STATIC_INLINE_PREFIX MPI_Aint MPIR_Csel_mean_count(const MPI_Aint * counts, int n)
{
    MPI_Aint sum = 0;
    for (int i = 0; i < n; i++) {
        sum += counts[i];
    }
    return (sum + n - 1) / n;
}

/* The message size in bytes. Only applicable to coll_types that every rank can have the same
 * size. Asserts for barrier, gatherv, scatterv, alltoallv, alltoallw, the neighborhood
 * collectives, and the intercommunicator allgatherv and reduce_scatter (the two groups hold
 * different recvcounts). */
MPL_STATIC_INLINE_PREFIX MPI_Aint MPIR_Csel_msg_size(MPIR_Csel_coll_sig_s * coll_sig)
{
    MPI_Datatype datatype = MPI_DATATYPE_NULL;
    MPI_Aint count = 0, type_size;
    int comm_size = coll_sig->comm_ptr->local_size;

    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(ALLREDUCE):
            datatype = coll_sig->u.allreduce.datatype;
            count = coll_sig->u.allreduce.count;
            break;
          COLL_TYPE_ALL_CASE(BCAST):
            datatype = coll_sig->u.bcast.datatype;
            count = coll_sig->u.bcast.count;
            break;
          COLL_TYPE_ALL_CASE(REDUCE):
            datatype = coll_sig->u.reduce.datatype;
            count = coll_sig->u.reduce.count;
            break;
          COLL_TYPE_ALL_CASE(ALLTOALL):
            datatype = coll_sig->u.alltoall.recvtype;
            count = coll_sig->u.alltoall.recvcount;
            break;
          COLL_TYPE_ALL_CASE(ALLGATHER):
            /* the receive side is valid also with MPI_IN_PLACE */
            datatype = coll_sig->u.allgather.recvtype;
            count = coll_sig->u.allgather.recvcount;
            break;
          COLL_TYPE_INTRA_CASE(ALLGATHERV):
            datatype = coll_sig->u.allgatherv.recvtype;
            count = MPIR_Csel_mean_count(coll_sig->u.allgatherv.recvcounts, comm_size);
            break;
          COLL_TYPE_ALL_CASE(GATHER):
            /* the receive side is valid only at the root, the send side elsewhere and at an
             * intercommunicator's root (MPI_ROOT), except at a root using MPI_IN_PLACE */
            if (coll_sig->u.gather.root == MPI_ROOT ||
                (coll_sig->comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                 coll_sig->u.gather.root == coll_sig->comm_ptr->rank)) {
                datatype = coll_sig->u.gather.recvtype;
                count = coll_sig->u.gather.recvcount;
            } else {
                datatype = coll_sig->u.gather.sendtype;
                count = coll_sig->u.gather.sendcount;
            }
            break;
          COLL_TYPE_ALL_CASE(SCATTER):
            if (coll_sig->u.scatter.root == MPI_ROOT ||
                (coll_sig->comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                 coll_sig->u.scatter.root == coll_sig->comm_ptr->rank)) {
                datatype = coll_sig->u.scatter.sendtype;
                count = coll_sig->u.scatter.sendcount;
            } else {
                datatype = coll_sig->u.scatter.recvtype;
                count = coll_sig->u.scatter.recvcount;
            }
            break;
          COLL_TYPE_INTRA_CASE(REDUCE_SCATTER):
            datatype = coll_sig->u.reduce_scatter.datatype;
            count = MPIR_Csel_mean_count(coll_sig->u.reduce_scatter.recvcounts, comm_size);
            break;
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER_BLOCK):
            datatype = coll_sig->u.reduce_scatter_block.datatype;
            count = coll_sig->u.reduce_scatter_block.recvcount;
            break;
          COLL_TYPE_ALL_CASE(SCAN):
            datatype = coll_sig->u.scan.datatype;
            count = coll_sig->u.scan.count;
            break;
          COLL_TYPE_ALL_CASE(EXSCAN):
            datatype = coll_sig->u.exscan.datatype;
            count = coll_sig->u.exscan.count;
            break;
        default:
            MPIR_Assert(0);
            return 0;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);
    return type_size * count;
}

/* boolean csel checker_functions */
MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_comm_size_is_pof2(MPIR_Csel_coll_sig_s * coll_sig)
{
    return MPL_is_pof2(coll_sig->comm_ptr->local_size);
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_block_regular(MPIR_Csel_coll_sig_s * coll_sig)
{
    const MPI_Aint *counts = NULL;
    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER):
            counts = coll_sig->u.reduce_scatter.recvcounts;
            break;
        default:
            MPIR_Assert(0);
            return false;
    }

    for (int i = 1; i < coll_sig->comm_ptr->local_size; i++) {
        if (counts[i] != counts[0]) {
            return false;
        }
    }
    return true;
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_displs_ordered(MPIR_Csel_coll_sig_s * coll_sig)
{
    const MPI_Aint *counts = NULL;
    const MPI_Aint *displs = NULL;
    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(ALLGATHERV):
            counts = coll_sig->u.allgatherv.recvcounts;
            displs = coll_sig->u.allgatherv.displs;
            break;
        default:
            return false;
    }

    MPI_Aint pos = 0;
    for (int i = 0; i < coll_sig->comm_ptr->local_size; i++) {
        if (pos != displs[i])
            return false;
        pos += counts[i];
    }
    return true;
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_sendbuf_inplace(MPIR_Csel_coll_sig_s * coll_sig)
{
    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(ALLTOALL):
            return (coll_sig->u.alltoall.sendbuf == MPI_IN_PLACE);
          COLL_TYPE_ALL_CASE(ALLTOALLV):
            return (coll_sig->u.alltoallv.sendbuf == MPI_IN_PLACE);
          COLL_TYPE_ALL_CASE(ALLTOALLW):
            return (coll_sig->u.alltoallw.sendbuf == MPI_IN_PLACE);
        default:
            fprintf(stderr, "is_sendbuf_inplace not defined for coll_type %d\n",
                    coll_sig->coll_type);
            MPIR_Assert(0);
            return false;
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_op_is_builtin(MPIR_Csel_coll_sig_s * coll_sig)
{
    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(ALLREDUCE):
            return HANDLE_IS_BUILTIN(coll_sig->u.allreduce.op);
          COLL_TYPE_ALL_CASE(REDUCE):
            return HANDLE_IS_BUILTIN(coll_sig->u.reduce.op);
            break;
        default:
            fprintf(stderr, "is_op_builtin not defined for coll_type %d\n", coll_sig->coll_type);
            MPIR_Assert(0);
            return false;
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_op_is_commutative(MPIR_Csel_coll_sig_s * coll_sig)
{
    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(ALLREDUCE):
            return MPIR_Op_is_commutative(coll_sig->u.allreduce.op);
          COLL_TYPE_ALL_CASE(REDUCE):
            return MPIR_Op_is_commutative(coll_sig->u.reduce.op);
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER):
            return MPIR_Op_is_commutative(coll_sig->u.reduce_scatter.op);
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER_BLOCK):
            return MPIR_Op_is_commutative(coll_sig->u.reduce_scatter_block.op);
          COLL_TYPE_ALL_CASE(SCAN):
            return MPIR_Op_is_commutative(coll_sig->u.scan.op);
        default:
            fprintf(stderr, "is_commutative not defined for coll_type %d\n", coll_sig->coll_type);
            MPIR_Assert(0);
            return false;
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_is_hierarchical(MPIR_Csel_coll_sig_s * coll_sig)
{
    return MPIR_Comm_is_parent_comm(coll_sig->comm_ptr);
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_is_node_consecutive(MPIR_Csel_coll_sig_s * coll_sig)
{
    return MPII_Comm_is_node_consecutive(coll_sig->comm_ptr);
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_is_node_canonical(MPIR_Csel_coll_sig_s * coll_sig)
{
    return MPII_Comm_is_node_canonical(coll_sig->comm_ptr);
}

/* a buffer this rank uses is device memory */
MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_is_gpu(MPIR_Csel_coll_sig_s * coll_sig)
{
    return coll_sig->flags & MPIR_COLL_SIG_FLAG__GPU;
}

/* every buffer this rank uses is device memory, and the user asserts all ranks do the same */
MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_is_gpu_coll(MPIR_Csel_coll_sig_s * coll_sig)
{
    return coll_sig->flags & MPIR_COLL_SIG_FLAG__GPU_COLL;
}

#include "coll_autogen.h"

#endif /* COLL_CSEL_H_INCLUDED */
