/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
*  (C) 2019 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

/* i-collective MPIR functions with transport based algorithms.
* The actual algorithms are declared in coll_tsp_algos.h and
* implemented in icoll/icoll_tsp_xxx_algos.h, where icoll refers
* to ibcast, igather, etc.
*/

#include "mpiimpl.h"

/* Declare all transport-namespace algorithms */
#include "tsp_gentran.h"        /* Gentran_ namespace */
#include "coll_tsp_algos.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_intra_recexch
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_intra_recexch(PARAMS_BARRIER, MPIR_Request ** request)
{
    return MPII_Gentran_Iallreduce_intra_recexch(MPI_IN_PLACE, NULL, 0, MPI_BYTE, MPI_SUM, comm_ptr,
                                                 MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                 MPIR_CVAR_IBARRIER_RECEXCH_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_intra_tree
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_intra_tree(PARAMS_BCAST, MPIR_Request ** request)
{
    return MPII_Gentran_Ibcast_intra_tree(buffer, count, datatype, root, comm_ptr,
                                          MPIR_Ibcast_tree_type, MPIR_CVAR_IBCAST_TREE_KVAL,
                                          MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE, request);
}

/* Ring algorithm is equivalent to k-ary tree algorithm with k = 1 */

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_intra_ring
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_intra_ring(PARAMS_BCAST, MPIR_Request ** request)
{
    return MPII_Gentran_Ibcast_intra_tree(buffer, count, datatype, root, comm_ptr,
                                          MPIR_TREE_TYPE_KARY, 1, MPIR_CVAR_IBCAST_RING_CHUNK_SIZE,
                                          request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_intra_tree
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_intra_tree(PARAMS_REDUCE, MPIR_Request ** request)
{
    return MPII_Gentran_Ireduce_intra_tree(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                           MPIR_Ibcast_tree_type, MPIR_CVAR_IREDUCE_TREE_KVAL,
                                           MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE, request);
}

/* Ring algorithm is equivalent to k-ary tree algorithm with k = 1 */

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_intra_ring
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_intra_ring(PARAMS_REDUCE, MPIR_Request ** request)
{
    return MPII_Gentran_Ireduce_intra_tree(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                           MPIR_TREE_TYPE_KARY, 1,
                                           MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_intra_scatter_recexch_allgather
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_intra_scatter_recexch_allgather(PARAMS_BCAST, MPIR_Request ** request)
{
    return MPII_Gentran_Ibcast_intra_scatter_recexch_allgather(buffer, count, datatype, root,
                                                               comm_ptr, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igather_intra_tree
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Igather_intra_tree(PARAMS_GATHER, MPIR_Request ** request)
{
    return MPII_Gentran_Igather_intra_tree(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, MPIR_CVAR_IGATHER_TREE_KVAL,
                                           request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_intra_tree
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_intra_tree(PARAMS_SCATTER, MPIR_Request ** request)
{
    return MPII_Gentran_Iscatter_intra_tree(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, root, comm_ptr, MPIR_CVAR_ISCATTER_TREE_KVAL,
                                            request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_intra_gentran_brucks
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_intra_gentran_brucks(PARAMS_ALLGATHER, MPIR_Request ** request)
{
    return MPII_Gentran_Iallgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr,
                                                MPIR_CVAR_IALLGATHER_BRUCKS_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_intra_gentran_ring
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_intra_gentran_ring(PARAMS_ALLGATHERV, MPIR_Request ** request)
{
    return MPII_Gentran_Iallgatherv_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcnts,
                                               rdispls, recvtype, comm_ptr, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_intra_recexch_distance_doubling
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_intra_recexch_distance_doubling(PARAMS_ALLGATHER, MPIR_Request ** request)
{
    return MPII_Gentran_Iallgather_intra_recexch(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, comm_ptr,
                                                 MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                 MPIR_CVAR_IALLGATHER_RECEXCH_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_intra_recexch_distance_halving
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_intra_recexch_distance_halving(PARAMS_ALLGATHER, MPIR_Request ** request)
{
    return MPII_Gentran_Iallgather_intra_recexch(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, comm_ptr,
                                                 MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING,
                                                 MPIR_CVAR_IALLGATHER_RECEXCH_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_intra_recexch_distance_doubling
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_intra_recexch_distance_doubling(PARAMS_ALLGATHERV, MPIR_Request ** request)
{
    return MPII_Gentran_Iallgatherv_intra_recexch(sendbuf, sendcount, sendtype, recvbuf, recvcnts,
                                                  rdispls, recvtype, comm_ptr,
                                                  MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                  MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_intra_recexch_distance_halving
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_intra_recexch_distance_halving(PARAMS_ALLGATHERV, MPIR_Request ** request)
{
    return MPII_Gentran_Iallgatherv_intra_recexch(sendbuf, sendcount, sendtype, recvbuf, recvcnts,
                                                  rdispls, recvtype, comm_ptr,
                                                  MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_HALVING,
                                                  MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra_tree_kary
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra_tree_kary(PARAMS_ALLREDUCE, MPIR_Request ** request)
{
    return MPII_Gentran_Iallreduce_intra_tree(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                              MPIR_TREE_TYPE_KARY, MPIR_CVAR_IALLREDUCE_TREE_KVAL,
                                              MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE,
                                              request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra_tree_knomial
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra_tree_knomial(PARAMS_ALLREDUCE, MPIR_Request ** request)
{
    return MPII_Gentran_Iallreduce_intra_tree(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                              MPIR_TREE_TYPE_KNOMIAL_1,
                                              MPIR_CVAR_IALLREDUCE_TREE_KVAL,
                                              MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE,
                                              request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra_recexch_single_buffer
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra_recexch_single_buffer(PARAMS_ALLREDUCE, MPIR_Request ** request)
{
    return MPII_Gentran_Iallreduce_intra_recexch(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                 MPIR_IALLREDUCE_RECEXCH_TYPE_SINGLE_BUFFER,
                                                 MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra_recexch_multiple_buffer
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra_recexch_multiple_buffer(PARAMS_ALLREDUCE, MPIR_Request ** request)
{
    return MPII_Gentran_Iallreduce_intra_recexch(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                 MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                 MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL, request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_intra_recexch
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_intra_recexch(PARAMS_REDUCE_SCATTER, MPIR_Request ** request)
{
    return MPII_Gentran_Ireduce_scatter_intra_recexch(sendbuf, recvbuf, recvcnts, datatype, op,
                                                      comm_ptr,
                                                      MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL,
                                                      request);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_intra_recexch
#undef FCNAME
#define FCNAMP MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_intra_recexch(PARAMS_REDUCE_SCATTER_BLOCK, MPIR_Request ** request)
{
    return MPII_Gentran_Ireduce_scatter_block_intra_recexch(sendbuf, recvbuf, count, datatype, op,
                                                            comm_ptr,
                                                            MPIR_CVAR_IREDUCE_SCATTER_BLOCK_RECEXCH_KVAL,
                                                            request);
}
