/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef COLL_CSEL_H_INCLUDED
#define COLL_CSEL_H_INCLUDED

#include "json.h"

typedef struct csel_node {
    MPIR_Csel_node_type_e type;
    union {
        struct {
            MPIR_Csel_coll_type_e coll_type;
        } collective;
        struct {
            bool negate;
            int thresh;
        } condition;
        MPII_Csel_container_s *container;
    } u;
    struct csel_node *success;
    struct csel_node *failure;
} MPIR_Csel_node_s;

extern MPIR_Coll_algo_fn *MPIR_Coll_algo_table;
extern int *MPIR_Coll_cvar_table;
extern const char **MPIR_Coll_type_names;
extern const char **MPIR_Coll_algo_names;
extern const char **MPIR_Csel_condition_names;

int MPIR_Csel_create_from_file(const char *json_file, void **csel);
int MPIR_Csel_create_from_buf(const char *json, void **csel);
int MPIR_Csel_free(void *csel);
MPII_Csel_container_s *MPIR_Csel_search(void *csel, MPIR_Csel_coll_sig_s * coll_sig);
void MPIR_Csel_print_tree(MPIR_Csel_node_s * node, int level);

MPL_STATIC_INLINE_PREFIX int MPIR_Csel_comm_size(MPIR_Csel_coll_sig_s * coll_sig)
{
    /* FIXME: update when we have intercomm algorithms that need select on comm_size */
    MPIR_Assert(coll_sig->comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    return coll_sig->comm_ptr->local_size;
}

#define COLL_TYPE_ALL_CASE(NAME) \
    case MPIR_CSEL_COLL_TYPE__INTRA_  ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTRA_I ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTER_  ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTER_I ## NAME

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIR_Csel_avg_msg_size(MPIR_Csel_coll_sig_s * coll_sig)
{
    MPI_Aint msgsize = 0;
    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(ALLREDUCE):
            MPIR_Datatype_get_size_macro(coll_sig->u.allreduce.datatype, msgsize);
            msgsize *= coll_sig->u.allreduce.count;
            break;
          COLL_TYPE_ALL_CASE(BCAST):
            MPIR_Datatype_get_size_macro(coll_sig->u.bcast.datatype, msgsize);
            msgsize *= coll_sig->u.bcast.count;
            break;
          COLL_TYPE_ALL_CASE(REDUCE):
            MPIR_Datatype_get_size_macro(coll_sig->u.reduce.datatype, msgsize);
            msgsize *= coll_sig->u.reduce.count;
            break;
          COLL_TYPE_ALL_CASE(ALLTOALL):
            MPIR_Datatype_get_size_macro(coll_sig->u.alltoall.sendtype, msgsize);
            msgsize *= coll_sig->u.alltoall.sendcount;
            break;
        default:
            fprintf(stderr, "avg_msg_size not defined for coll_type %d\n", coll_sig->coll_type);
            MPIR_Assert(0);
            break;
    }
    return msgsize;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIR_Csel_total_msg_size(MPIR_Csel_coll_sig_s * coll_sig)
{
    MPI_Aint total_bytes = 0;
    int comm_size = coll_sig->comm_ptr->local_size;

    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(ALLREDUCE):
            MPIR_Datatype_get_size_macro(coll_sig->u.allreduce.datatype, total_bytes);
            total_bytes *= coll_sig->u.allreduce.count * comm_size;
            break;
          COLL_TYPE_ALL_CASE(BCAST):
            MPIR_Datatype_get_size_macro(coll_sig->u.bcast.datatype, total_bytes);
            total_bytes *= coll_sig->u.bcast.count * comm_size;
            break;
          COLL_TYPE_ALL_CASE(REDUCE):
            MPIR_Datatype_get_size_macro(coll_sig->u.reduce.datatype, total_bytes);
            total_bytes *= coll_sig->u.reduce.count * comm_size;
            break;
          COLL_TYPE_ALL_CASE(ALLTOALL):
            MPIR_Datatype_get_size_macro(coll_sig->u.alltoall.sendtype, total_bytes);
            total_bytes *= coll_sig->u.alltoall.sendcount * comm_size;
            break;
          COLL_TYPE_ALL_CASE(ALLTOALLV):
            MPIR_Datatype_get_size_macro(coll_sig->u.alltoallv.sendtype, total_bytes);
            {
                MPI_Aint count = 0;
                for (int i = 0; i < comm_size; i++) {
                    count += coll_sig->u.alltoallv.sendcounts[i];
                }
                total_bytes *= count;
            }
            break;
          COLL_TYPE_ALL_CASE(ALLTOALLW):
            for (int i = 0; i < comm_size; i++) {
                MPI_Aint typesize;
                MPIR_Datatype_get_size_macro(coll_sig->u.alltoallw.sendtypes[i], typesize);
                total_bytes += (coll_sig->u.alltoallw.sendcounts[i] * typesize);
            }
            break;
          COLL_TYPE_ALL_CASE(ALLGATHER):
            MPIR_Datatype_get_size_macro(coll_sig->u.allgather.recvtype, total_bytes);
            total_bytes *= coll_sig->u.allgather.recvcount * comm_size;
            break;
          COLL_TYPE_ALL_CASE(ALLGATHERV):
            MPIR_Datatype_get_size_macro(coll_sig->u.allgatherv.recvtype, total_bytes);
            {
                MPI_Aint count = 0;
                for (int i = 0; i < comm_size; i++) {
                    count += coll_sig->u.allgatherv.recvcounts[i];
                }
                total_bytes *= count;
            }
            break;
          COLL_TYPE_ALL_CASE(GATHER):
            if (coll_sig->u.gather.root == MPI_ROOT) {
                MPIR_Datatype_get_size_macro(coll_sig->u.gather.recvtype, total_bytes);
                /* use remote size for intercomm root */
                if (coll_sig->comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    total_bytes = coll_sig->u.gather.recvcount * (coll_sig->comm_ptr->remote_size);
                else
                    total_bytes = coll_sig->u.gather.recvcount * comm_size;
            } else {
                MPIR_Datatype_get_size_macro(coll_sig->u.gather.sendtype, total_bytes);
                total_bytes = coll_sig->u.gather.sendcount * comm_size;
            }
            break;
          COLL_TYPE_ALL_CASE(SCATTER):
            if (coll_sig->u.scatter.root == MPI_ROOT) {
                MPIR_Datatype_get_size_macro(coll_sig->u.scatter.sendtype, total_bytes);
                /* use remote size for intercomm root */
                if (coll_sig->comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    total_bytes = coll_sig->u.scatter.sendcount * (coll_sig->comm_ptr->remote_size);
                else
                    total_bytes = coll_sig->u.scatter.sendcount * comm_size;
            } else {
                MPIR_Datatype_get_size_macro(coll_sig->u.scatter.recvtype, total_bytes);
                total_bytes = coll_sig->u.scatter.recvcount * comm_size;
            }
            break;
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER):
            MPIR_Datatype_get_size_macro(coll_sig->u.reduce_scatter.datatype, total_bytes);
            {
                MPI_Aint count = 0;
                for (int i = 0; i < comm_size; i++) {
                    count += coll_sig->u.reduce_scatter.recvcounts[i];
                }
                total_bytes *= count;
            }
            break;
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER_BLOCK):
            MPIR_Datatype_get_size_macro(coll_sig->u.reduce_scatter_block.datatype, total_bytes);
            total_bytes *= coll_sig->u.reduce_scatter_block.recvcount * comm_size;
            break;
        default:
            MPIR_Assert(0);
            break;
    }

    return total_bytes;
}

/* boolean csel checker_functions */
MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_comm_size_is_pof2(MPIR_Csel_coll_sig_s * coll_sig)
{
    return MPL_is_pof2(coll_sig->comm_ptr->local_size);
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_count_ge_pof2(MPIR_Csel_coll_sig_s * coll_sig)
{
    switch (coll_sig->coll_type) {
          COLL_TYPE_ALL_CASE(REDUCE):
            return (coll_sig->u.reduce.count >= MPL_pof2(coll_sig->comm_ptr->local_size));
          COLL_TYPE_ALL_CASE(ALLREDUCE):
            return (coll_sig->u.allreduce.count >= MPL_pof2(coll_sig->comm_ptr->local_size));
        default:
            printf("MPIR_Csel_count_ge_pof2: unsupported coll_type: %s\n",
                   MPIR_Coll_type_names[coll_sig->coll_type]);
            MPIR_Assert(0);
            return false;
    }
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

#include "coll_autogen.h"

#endif /* COLL_CSEL_H_INCLUDED */
