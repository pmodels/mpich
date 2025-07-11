/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "hcoll.h"

int hcoll_Barrier(MPIR_Comm * comm_ptr, int coll_attr)
{
    int rc = -1;
    MPI_Comm comm = comm_ptr->handle;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOL BARRIER.");
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    rc = hcoll_collectives.coll_barrier(comm_ptr->hcoll_priv.hcoll_context);
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    return rc;
}

int hcoll_Bcast(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                MPIR_Comm * comm_ptr, int coll_attr)
{
    dte_data_representation_t dtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL BCAST.");
    dtype = mpi_dtype_2_hcoll_dtype(datatype, count, TRY_FIND_DERIVED);
    MPI_Comm comm = comm_ptr->handle;
    if (HCOL_DTE_IS_COMPLEX(dtype) || HCOL_DTE_IS_ZERO(dtype)) {
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "unsupported data layout, calling fallback bcast.");
        rc = -1;
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        rc = hcoll_collectives.coll_bcast(buffer, count, dtype, root,
                                          comm_ptr->hcoll_priv.hcoll_context);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    }
    return rc;
}

int hcoll_Reduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                 MPI_Op op, int root, MPIR_Comm * comm_ptr, int coll_attr)
{
    dte_data_representation_t dtype;
    hcoll_dte_op_t *Op;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL REDUCE.");
    dtype = mpi_dtype_2_hcoll_dtype(datatype, count, TRY_FIND_DERIVED);
    Op = mpi_op_2_dte_op(op);
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
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        rc = hcoll_collectives.coll_reduce((void *) sendbuf, recvbuf, count, dtype, Op, root,
                                           comm_ptr->hcoll_priv.hcoll_context);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    }
    return rc;
}

int hcoll_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                    MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc = -1;
    MPI_Comm comm = comm_ptr->handle;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOL ALLREDUCE.");
    Dtype = mpi_dtype_2_hcoll_dtype(datatype, count, TRY_FIND_DERIVED);
    Op = mpi_op_2_dte_op(op);
    if (MPI_IN_PLACE == sendbuf) {
        sendbuf = HCOLL_IN_PLACE;
    }
    if (HCOL_DTE_IS_COMPLEX(Dtype) || HCOL_DTE_IS_ZERO(Dtype) || (HCOL_DTE_OP_NULL == Op->id)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout, calling fallback allreduce.");
        rc = -1;
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        rc = hcoll_collectives.coll_allreduce((void *) sendbuf, recvbuf, count, Dtype, Op,
                                              comm_ptr->hcoll_priv.hcoll_context);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    }
    return rc;
}

int hcoll_Allgather(const void *sbuf, int scount, MPI_Datatype sdtype,
                    void *rbuf, int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                    int coll_attr)
{
    MPI_Comm comm = comm_ptr->handle;
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL ALLGATHER.");
    rtype = mpi_dtype_2_hcoll_dtype(rdtype, rcount, TRY_FIND_DERIVED);
    if (MPI_IN_PLACE == sbuf) {
        sbuf = HCOLL_IN_PLACE;
        stype = rtype;
    } else {
        stype = mpi_dtype_2_hcoll_dtype(sdtype, rcount, TRY_FIND_DERIVED);
    }
    if (HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype) ||
        HCOL_DTE_IS_COMPLEX(rtype)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout; calling fallback allgather.");
        rc = -1;
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        rc = hcoll_collectives.coll_allgather((void *) sbuf, scount, stype, rbuf, rcount, rtype,
                                              comm_ptr->hcoll_priv.hcoll_context);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    }
    return rc;
}

int hcoll_Alltoall(const void *sbuf, int scount, MPI_Datatype sdtype,
                   void *rbuf, int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr, int coll_attr)
{
    MPI_Comm comm = comm_ptr->handle;
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL ALLGATHER.");
    rtype = mpi_dtype_2_hcoll_dtype(rdtype, rcount, TRY_FIND_DERIVED);
    if (MPI_IN_PLACE == sbuf) {
        sbuf = HCOLL_IN_PLACE;
        stype = rtype;
    } else {
        stype = mpi_dtype_2_hcoll_dtype(sdtype, rcount, TRY_FIND_DERIVED);
    }
    if (HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype) ||
        HCOL_DTE_IS_COMPLEX(rtype)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout; calling fallback allgather.");
        rc = -1;
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        rc = hcoll_collectives.coll_alltoall((void *) sbuf, scount, stype, rbuf, rcount, rtype,
                                             comm_ptr->hcoll_priv.hcoll_context);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    }
    return rc;
}

int hcoll_Alltoallv(const void *sbuf, const MPI_Aint * scounts, const MPI_Aint * sdispls,
                    MPI_Datatype sdtype, void *rbuf, const MPI_Aint * rcounts,
                    const MPI_Aint * rdispls, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                    int coll_attr)
{
    MPI_Comm comm = comm_ptr->handle;
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL ALLGATHER.");
    rtype = mpi_dtype_2_hcoll_dtype(rdtype, 0, TRY_FIND_DERIVED);
    if (MPI_IN_PLACE == sbuf) {
        sbuf = HCOLL_IN_PLACE;
        stype = rtype;
    } else {
        stype = mpi_dtype_2_hcoll_dtype(sdtype, 0, TRY_FIND_DERIVED);
    }
    if (HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype) ||
        HCOL_DTE_IS_COMPLEX(rtype)) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout; calling fallback allgather.");
        rc = -1;
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
        rc = hcoll_collectives.coll_alltoallv((void *) sbuf, (int *) scounts, (int *) sdispls,
                                              stype, rbuf, (int *) rcounts, (int *) rdispls, rtype,
                                              comm_ptr->hcoll_priv.hcoll_context);
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_HCOLL_MUTEX);
    }
    return rc;
}
