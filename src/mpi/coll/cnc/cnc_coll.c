/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include <cnc/collective.h>
#include <cnc/cnc_oob_mpi.h>
#include "cnc_internal.h"

/* Sentinel stored in comm->coll.cnc_priv on the private dup'd communicator
 * while cnc_init() runs its internal MPI collectives, preventing recursive
 * entry into the cnc algorithms. */
#define CNC_PRIV_SKIP ((void *) -1)

struct cnc_comm_priv {
    MPIR_Comm *dup_comm;
    cnc_oob_t oob;
    cnc_transport_t *transport;
};

static int cnc_get_transport(MPIR_Comm * comm_ptr, cnc_transport_t ** t_out)
{
    int mpi_errno = MPI_SUCCESS;
    struct cnc_comm_priv *priv = comm_ptr->coll.cnc_priv;

    if (priv == CNC_PRIV_SKIP)
        return MPI_ERR_OTHER;
    if (priv) {
        *t_out = priv->transport;
        return MPI_SUCCESS;
    }

    priv = MPL_calloc(1, sizeof(*priv), MPL_MEM_COLL);
    if (!priv)
        return MPI_ERR_OTHER;

    /* MPII_Comm_copy allocates a dedicated context ID without copying user
     * communicator attributes or invoking attribute callbacks. */
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_ptr->local_size, NULL, &priv->dup_comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPL_free(priv);
        return mpi_errno;
    }

    priv->dup_comm->coll.cnc_priv = CNC_PRIV_SKIP;

    if (cnc_oob_mpi_create(&priv->oob, priv->dup_comm->handle) != CNC_OK) {
        MPIR_Comm_release(priv->dup_comm);
        MPL_free(priv);
        return MPI_ERR_OTHER;
    }

    if (cnc_init(&priv->transport, &priv->oob) != CNC_OK) {
        cnc_oob_mpi_destroy(&priv->oob);
        MPIR_Comm_release(priv->dup_comm);
        MPL_free(priv);
        return MPI_ERR_OTHER;
    }

    comm_ptr->coll.cnc_priv = priv;
    *t_out = priv->transport;
    return MPI_SUCCESS;
}

int MPII_Cnc_comm_cleanup(MPIR_Comm * comm_ptr)
{
    struct cnc_comm_priv *priv = comm_ptr->coll.cnc_priv;

    if (!priv || priv == CNC_PRIV_SKIP) {
        comm_ptr->coll.cnc_priv = NULL;
        return MPI_SUCCESS;
    }

    comm_ptr->coll.cnc_priv = NULL;
    cnc_fini(priv->transport);
    cnc_oob_mpi_destroy(&priv->oob);
    MPIR_Comm_release(priv->dup_comm);
    MPL_free(priv);
    return MPI_SUCCESS;
}

static int cnc_is_f32(MPI_Datatype datatype)
{
    return datatype == MPI_FLOAT;
}

static int cnc_is_i32(MPI_Datatype datatype)
{
    return datatype == MPI_INT || datatype == MPI_INT32_T;
}

static int cnc_contig_host(const void *buf, MPI_Datatype datatype)
{
    int is_contig;

    MPIR_Datatype_is_contig(datatype, &is_contig);
    return is_contig && !MPIR_GPU_query_pointer_is_dev(buf);
}

int MPIR_Bcast_intra_cnc(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint type_size, true_lb;

    if (MPIR_ThreadInfo.isThreaded || comm_ptr->coll.cnc_priv == CNC_PRIV_SKIP ||
        !cnc_contig_host(buffer, datatype))
        return MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, coll_attr);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_true_lb(datatype, &true_lb);

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_bcast(CNC_IN_PLACE, (char *) buffer + true_lb, count * type_size,
                   comm_ptr->local_size, comm_ptr->rank, root, t, NULL);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;
}

int MPIR_Allreduce_intra_cnc(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    cnc_allreduce_opts_t opts;
    cnc_datatype_t dt;

    if (MPIR_ThreadInfo.isThreaded || comm_ptr->coll.cnc_priv == CNC_PRIV_SKIP)
        goto fallback;

    if (cnc_is_f32(datatype))
        dt = CNC_DTYPE_F32;
    else if (cnc_is_i32(datatype))
        dt = CNC_DTYPE_I32;
    else
        goto fallback;

    memset(&opts, 0, sizeof(opts));
    if (op == MPI_SUM)
        opts.op = CNC_REDUCE_SUM;
    else if (op == MPI_PROD)
        opts.op = CNC_REDUCE_PROD;
    else if (op == MPI_MIN)
        opts.op = CNC_REDUCE_MIN;
    else if (op == MPI_MAX)
        opts.op = CNC_REDUCE_MAX;
    else
        goto fallback;

    if ((sendbuf != MPI_IN_PLACE && !cnc_contig_host(sendbuf, datatype)) ||
        !cnc_contig_host(recvbuf, datatype))
        goto fallback;

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_allreduce(sendbuf == MPI_IN_PLACE ? CNC_IN_PLACE : sendbuf, recvbuf, count, dt,
                       comm_ptr->local_size, comm_ptr->rank, t, &opts);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                   comm_ptr, coll_attr);
}
