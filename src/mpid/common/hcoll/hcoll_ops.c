/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "hcoll.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
{
    int rc = -1;
    MPI_Comm comm = comm_ptr->handle;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOL BARRIER.");
    rc = hcoll_collectives.coll_barrier(comm_ptr->hcoll_priv.hcoll_context);
    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
{
    dte_data_representation_t dtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL BCAST.");
    dtype = MPIDI_HCOLL_mpi_to_hcoll_dtype(datatype, count, TRY_FIND_DERIVED);
    MPI_Comm comm = comm_ptr->handle;
    if (HCOL_DTE_IS_COMPLEX(dtype) || HCOL_DTE_IS_ZERO(dtype)) {
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "unsupported data layout, calling fallback bcast.");
        rc = -1;
    } else {
        rc = hcoll_collectives.coll_bcast(buffer, count, dtype, root,
                                          comm_ptr->hcoll_priv.hcoll_context);
    }
    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                       MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
{
    dte_data_representation_t dtype;
    hcoll_dte_op_t *Op;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL REDUCE.");
    dtype = MPIDI_HCOLL_mpi_to_hcoll_dtype(datatype, count, TRY_FIND_DERIVED);
    Op = MPIDI_HCOLL_mpi_to_dte_op(op);
    if (MPI_IN_PLACE == sendbuf) {
        sendbuf = HCOLL_IN_PLACE;
    }
    if (HCOL_DTE_IS_COMPLEX(dtype) || HCOL_DTE_IS_ZERO(dtype) || (HCOL_DTE_OP_NULL == Op->id)) {
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "unsupported data layout, calling fallback bcast.");
        rc = -1;
    } else {
        rc = hcoll_collectives.coll_reduce((void *) sendbuf, recvbuf, count, dtype, Op, root,
                                           comm_ptr->hcoll_priv.hcoll_context);
    }
    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc = -1;
    MPI_Comm comm = comm_ptr->handle;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOL ALLREDUCE.");
    Dtype = MPIDI_HCOLL_mpi_to_hcoll_dtype(datatype, count, TRY_FIND_DERIVED);
    Op = MPIDI_HCOLL_mpi_to_dte_op(op);
    if (MPI_IN_PLACE == sendbuf) {
        sendbuf = HCOLL_IN_PLACE;
    }
    if (HCOL_DTE_IS_COMPLEX(Dtype) || HCOL_DTE_IS_ZERO(Dtype) || (HCOL_DTE_OP_NULL == Op->id)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout, calling fallback allreduce.");
        rc = -1;
    } else {
        rc = hcoll_collectives.coll_allreduce((void *) sendbuf, recvbuf, count, Dtype, Op,
                                              comm_ptr->hcoll_priv.hcoll_context);
    }
    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_allgather(const void *sbuf, int scount, MPI_Datatype sdtype,
                          void *rbuf, int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                          MPIR_Errflag_t * err)
{
    MPI_Comm comm = comm_ptr->handle;
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL ALLGATHER.");
    rtype = MPIDI_HCOLL_mpi_to_hcoll_dtype(rdtype, rcount, TRY_FIND_DERIVED);
    if (MPI_IN_PLACE == sbuf) {
        sbuf = HCOLL_IN_PLACE;
        stype = rtype;
    } else {
        stype = MPIDI_HCOLL_mpi_to_hcoll_dtype(sdtype, rcount, TRY_FIND_DERIVED);
    }
    if (HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype) ||
        HCOL_DTE_IS_COMPLEX(rtype)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout; calling fallback allgather.");
        rc = -1;
    } else {
        rc = hcoll_collectives.coll_allgather((void *) sbuf, scount, stype, rbuf, rcount, rtype,
                                              comm_ptr->hcoll_priv.hcoll_context);
    }
    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_alltoall(const void *sbuf, int scount, MPI_Datatype sdtype,
                         void *rbuf, int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                         MPIR_Errflag_t * err)
{
    MPI_Comm comm = comm_ptr->handle;
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL ALLGATHER.");
    rtype = MPIDI_HCOLL_mpi_to_hcoll_dtype(rdtype, rcount, TRY_FIND_DERIVED);
    if (MPI_IN_PLACE == sbuf) {
        sbuf = HCOLL_IN_PLACE;
        stype = rtype;
    } else {
        stype = MPIDI_HCOLL_mpi_to_hcoll_dtype(sdtype, rcount, TRY_FIND_DERIVED);
    }
    if (HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype) ||
        HCOL_DTE_IS_COMPLEX(rtype)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout; calling fallback allgather.");
        rc = -1;
    } else {
        rc = hcoll_collectives.coll_alltoall((void *) sbuf, scount, stype, rbuf, rcount, rtype,
                                             comm_ptr->hcoll_priv.hcoll_context);
    }
    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_HCOLL_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_HCOLL_alltoallv(const void *sbuf, const int *scounts, const int *sdispls,
                          MPI_Datatype sdtype, void *rbuf, const int *rcounts, const int *rdispls,
                          MPI_Datatype rdtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
{
    MPI_Comm comm = comm_ptr->handle;
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL ALLGATHER.");
    rtype = MPIDI_HCOLL_mpi_to_hcoll_dtype(rdtype, 0, TRY_FIND_DERIVED);
    if (MPI_IN_PLACE == sbuf) {
        sbuf = HCOLL_IN_PLACE;
        stype = rtype;
    } else {
        stype = MPIDI_HCOLL_mpi_to_hcoll_dtype(sdtype, 0, TRY_FIND_DERIVED);
    }
    if (HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype) ||
        HCOL_DTE_IS_COMPLEX(rtype)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout; calling fallback allgather.");
        rc = -1;
    } else {
        rc = hcoll_collectives.coll_alltoallv((void *) sbuf, (int *) scounts, (int *) sdispls,
                                              stype, rbuf, (int *) rcounts, (int *) rdispls, rtype,
                                              comm_ptr->hcoll_priv.hcoll_context);
    }
    return rc;
}
