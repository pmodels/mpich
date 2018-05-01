/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "hcoll.h"

#undef FUNCNAME
#define FUNCNAME hcoll_Barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_Barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
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
#define FUNCNAME hcoll_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
{
    dte_data_representation_t dtype;
    int rc = -1;

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL BCAST.");
    dtype = mpi_dtype_2_dte_dtype(datatype);
    int is_homogeneous = 1;
    MPI_Comm comm = comm_ptr->handle;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    if (HCOL_DTE_IS_COMPLEX(dtype) || HCOL_DTE_IS_ZERO(dtype) || (0 == is_homogeneous)) {
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
#define FUNCNAME hcoll_Allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                    MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * err)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc = -1;
    int is_homogeneous = 1;
    MPI_Comm comm = comm_ptr->handle;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOL ALLREDUCE.");
    Dtype = mpi_dtype_2_dte_dtype(datatype);
    Op = mpi_op_2_dte_op(op);
    if (MPI_IN_PLACE == sendbuf) {
        sendbuf = HCOLL_IN_PLACE;
    }
    if (HCOL_DTE_IS_COMPLEX(Dtype) || HCOL_DTE_IS_ZERO(Dtype) || (0 == is_homogeneous) ||
        (HCOL_DTE_OP_NULL == Op->id)) {
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
#define FUNCNAME hcoll_Allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int hcoll_Allgather(const void *sbuf, int scount, MPI_Datatype sdtype,
                    void *rbuf, int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                    MPIR_Errflag_t * err)
{
    int is_homogeneous = 1;
    MPI_Comm comm = comm_ptr->handle;
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc = -1;
    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    if (!comm_ptr->hcoll_priv.is_hcoll_init)
        return rc;

    MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE, "RUNNING HCOLL ALLGATHER.");
    stype = mpi_dtype_2_dte_dtype(sdtype);
    rtype = mpi_dtype_2_dte_dtype(rdtype);
    if (MPI_IN_PLACE == sbuf) {
        sbuf = HCOLL_IN_PLACE;
    }
    if (HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype) ||
        HCOL_DTE_IS_COMPLEX(rtype) || is_homogeneous == 0) {
        MPL_DBG_MSG(MPIR_DBG_HCOLL, VERBOSE,
                    "unsupported data layout; calling fallback allgather.");
        rc = -1;
    } else {
        rc = hcoll_collectives.coll_allgather((void *) sbuf, scount, stype, rbuf, rcount, rtype,
                                              comm_ptr->hcoll_priv.hcoll_context);
    }
    return rc;
}
