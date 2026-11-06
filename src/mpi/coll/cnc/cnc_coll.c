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

static int cnc_can_run(MPIR_Comm * comm_ptr)
{
    return !MPIR_ThreadInfo.isThreaded && comm_ptr->coll.cnc_priv != CNC_PRIV_SKIP;
}

static int cnc_contig_host(const void *buf, MPI_Datatype datatype)
{
    int is_contig;

    MPIR_Datatype_is_contig(datatype, &is_contig);
    return is_contig && !MPIR_GPU_query_pointer_is_dev(buf);
}

static int cnc_map_dtype_op(MPI_Datatype datatype, MPI_Op op, cnc_datatype_t * dt_out,
                            cnc_reduce_op_t * op_out)
{
    if (datatype == MPI_FLOAT)
        *dt_out = CNC_DTYPE_F32;
    else if (datatype == MPI_INT || datatype == MPI_INT32_T)
        *dt_out = CNC_DTYPE_I32;
    else
        return 0;

    if (op == MPI_SUM)
        *op_out = CNC_REDUCE_SUM;
    else if (op == MPI_PROD)
        *op_out = CNC_REDUCE_PROD;
    else if (op == MPI_MIN)
        *op_out = CNC_REDUCE_MIN;
    else if (op == MPI_MAX)
        *op_out = CNC_REDUCE_MAX;
    else
        return 0;

    return 1;
}

int MPIR_Barrier_intra_cnc(MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;

    if (!cnc_can_run(comm_ptr))
        return MPIR_Barrier_intra_dissemination(comm_ptr, coll_attr);

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    cnc_barrier(comm_ptr->local_size, comm_ptr->rank, t);
    return MPI_SUCCESS;
}

int MPIR_Bcast_intra_cnc(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint type_size, true_lb;

    if (!cnc_can_run(comm_ptr) || !cnc_contig_host(buffer, datatype))
        return MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, coll_attr);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_true_lb(datatype, &true_lb);

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_bcast(CNC_IN_PLACE, (char *) buffer + true_lb, count * type_size,
                   comm_ptr->local_size, comm_ptr->rank, root, t, NULL);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;
}

int MPIR_Gather_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                          void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                          MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint nbytes, send_lb = 0, recv_lb = 0;
    const void *sbuf_adj;
    void *rbuf_adj = NULL;
    int rank = comm_ptr->rank;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    if (rank == root) {
        MPI_Aint recv_size;
        if (!cnc_contig_host(recvbuf, recvtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(recvtype, recv_size);
        MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
        nbytes = recvcount * recv_size;
        rbuf_adj = (char *) recvbuf + recv_lb;
        if (sendbuf == MPI_IN_PLACE) {
            sbuf_adj = (const char *) rbuf_adj + rank * nbytes;
        } else {
            MPI_Aint send_size;
            if (!cnc_contig_host(sendbuf, sendtype))
                goto fallback;
            MPIR_Datatype_get_size_macro(sendtype, send_size);
            if (sendcount * send_size != nbytes)
                goto fallback;
            MPIR_Datatype_get_true_lb(sendtype, &send_lb);
            sbuf_adj = (const char *) sendbuf + send_lb;
        }
    } else {
        MPI_Aint send_size;
        if (!cnc_contig_host(sendbuf, sendtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        MPIR_Datatype_get_true_lb(sendtype, &send_lb);
        nbytes = sendcount * send_size;
        sbuf_adj = (const char *) sendbuf + send_lb;
    }

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_gather(sbuf_adj, rbuf_adj, nbytes, comm_ptr->local_size, rank, root, t, NULL);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                      root, comm_ptr, coll_attr);
}

int MPIR_Gatherv_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                           MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint send_lb = 0, recv_lb = 0;
    size_t send_nbytes;
    const void *sbuf_adj;
    void *rbuf_adj = NULL;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    size_t *rcounts_bytes = NULL;
    ptrdiff_t *rdispls_bytes = NULL;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    if (rank == root) {
        MPI_Aint recv_size, recv_extent;
        if (!cnc_contig_host(recvbuf, recvtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(recvtype, recv_size);
        MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
        MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
        rbuf_adj = (char *) recvbuf + recv_lb;
        for (int i = 0; i < n; i++) {
            if (recvcounts[i] < 0 || displs[i] * recv_extent < 0)
                goto fallback;
        }
        if (sendbuf == MPI_IN_PLACE) {
            send_nbytes = (size_t) (recvcounts[rank] * recv_size);
            sbuf_adj = (const char *) rbuf_adj + displs[rank] * recv_extent;
        } else {
            MPI_Aint send_size;
            if (!cnc_contig_host(sendbuf, sendtype) || sendcount < 0)
                goto fallback;
            MPIR_Datatype_get_size_macro(sendtype, send_size);
            if (sendcount * send_size != recvcounts[rank] * recv_size)
                goto fallback;
            MPIR_Datatype_get_true_lb(sendtype, &send_lb);
            send_nbytes = (size_t) (sendcount * send_size);
            sbuf_adj = (const char *) sendbuf + send_lb;
        }
        rcounts_bytes = MPL_malloc((size_t) n * sizeof(*rcounts_bytes), MPL_MEM_COLL);
        rdispls_bytes = MPL_malloc((size_t) n * sizeof(*rdispls_bytes), MPL_MEM_COLL);
        if (!rcounts_bytes || !rdispls_bytes) {
            MPL_free(rcounts_bytes);
            MPL_free(rdispls_bytes);
            return MPI_ERR_OTHER;
        }
        for (int i = 0; i < n; i++) {
            rcounts_bytes[i] = (size_t) (recvcounts[i] * recv_size);
            rdispls_bytes[i] = (ptrdiff_t) (displs[i] * recv_extent);
        }
    } else {
        MPI_Aint send_size;
        if (!cnc_contig_host(sendbuf, sendtype) || sendcount < 0)
            goto fallback;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        MPIR_Datatype_get_true_lb(sendtype, &send_lb);
        send_nbytes = (size_t) (sendcount * send_size);
        sbuf_adj = (const char *) sendbuf + send_lb;
        /* In MPI_Gatherv, recvcounts and displs are only valid at root; build a
         * local counts array with this rank's byte count at index rank. */
        rcounts_bytes = MPL_calloc((size_t) n, sizeof(*rcounts_bytes), MPL_MEM_COLL);
        if (!rcounts_bytes)
            return MPI_ERR_OTHER;
        rcounts_bytes[rank] = send_nbytes;
    }

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS) {
        MPL_free(rcounts_bytes);
        MPL_free(rdispls_bytes);
        return MPI_ERR_OTHER;
    }

    st = cnc_gatherv_linear(sbuf_adj, send_nbytes, rbuf_adj, rcounts_bytes, rdispls_bytes,
                            n, rank, root, t);
    MPL_free(rcounts_bytes);
    MPL_free(rdispls_bytes);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                       recvtype, root, comm_ptr, coll_attr);
}

int MPIR_Scatter_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                           void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                           MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint nbytes, send_lb = 0, recv_lb = 0;
    const void *sbuf_adj = NULL;
    void *rbuf_adj;
    int rank = comm_ptr->rank;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    if (rank == root) {
        MPI_Aint send_size;
        if (!cnc_contig_host(sendbuf, sendtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        MPIR_Datatype_get_true_lb(sendtype, &send_lb);
        nbytes = sendcount * send_size;
        sbuf_adj = (const char *) sendbuf + send_lb;
        if (recvbuf == MPI_IN_PLACE) {
            rbuf_adj = CNC_IN_PLACE;
        } else {
            MPI_Aint recv_size;
            if (!cnc_contig_host(recvbuf, recvtype))
                goto fallback;
            MPIR_Datatype_get_size_macro(recvtype, recv_size);
            if (recvcount * recv_size != nbytes)
                goto fallback;
            MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
            rbuf_adj = (char *) recvbuf + recv_lb;
        }
    } else {
        MPI_Aint recv_size;
        if (!cnc_contig_host(recvbuf, recvtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(recvtype, recv_size);
        MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
        nbytes = recvcount * recv_size;
        rbuf_adj = (char *) recvbuf + recv_lb;
    }

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_scatter(sbuf_adj, rbuf_adj, nbytes, comm_ptr->local_size, rank, root, t, NULL);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       root, comm_ptr, coll_attr);
}

int MPIR_Scatterv_intra_cnc(const void *sendbuf, const MPI_Aint * sendcounts,
                            const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                            MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint send_lb = 0, recv_lb = 0;
    size_t recv_nbytes;
    const void *sbuf_adj = NULL;
    void *rbuf_adj;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    size_t *scounts_bytes = NULL;
    ptrdiff_t *sdispls_bytes = NULL;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    if (rank == root) {
        MPI_Aint send_size, send_extent;
        if (!cnc_contig_host(sendbuf, sendtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        MPIR_Datatype_get_extent_macro(sendtype, send_extent);
        MPIR_Datatype_get_true_lb(sendtype, &send_lb);
        sbuf_adj = (const char *) sendbuf + send_lb;
        for (int i = 0; i < n; i++) {
            if (sendcounts[i] < 0 || displs[i] * send_extent < 0)
                goto fallback;
        }
        if (recvbuf == MPI_IN_PLACE) {
            recv_nbytes = (size_t) (sendcounts[rank] * send_size);
            rbuf_adj = (char *) sbuf_adj + displs[rank] * send_extent;
        } else {
            MPI_Aint recv_size;
            if (!cnc_contig_host(recvbuf, recvtype) || recvcount < 0)
                goto fallback;
            MPIR_Datatype_get_size_macro(recvtype, recv_size);
            if (recvcount * recv_size != sendcounts[rank] * send_size)
                goto fallback;
            MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
            recv_nbytes = (size_t) (recvcount * recv_size);
            rbuf_adj = (char *) recvbuf + recv_lb;
        }
        scounts_bytes = MPL_malloc((size_t) n * sizeof(*scounts_bytes), MPL_MEM_COLL);
        sdispls_bytes = MPL_malloc((size_t) n * sizeof(*sdispls_bytes), MPL_MEM_COLL);
        if (!scounts_bytes || !sdispls_bytes) {
            MPL_free(scounts_bytes);
            MPL_free(sdispls_bytes);
            return MPI_ERR_OTHER;
        }
        for (int i = 0; i < n; i++) {
            scounts_bytes[i] = (size_t) (sendcounts[i] * send_size);
            sdispls_bytes[i] = (ptrdiff_t) (displs[i] * send_extent);
        }
    } else {
        MPI_Aint recv_size;
        if (!cnc_contig_host(recvbuf, recvtype) || recvcount < 0)
            goto fallback;
        MPIR_Datatype_get_size_macro(recvtype, recv_size);
        MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
        recv_nbytes = (size_t) (recvcount * recv_size);
        rbuf_adj = (char *) recvbuf + recv_lb;
        /* In MPI_Scatterv, sendcounts and displs are only valid at root; build a
         * local counts array with this rank's byte count at index rank. */
        scounts_bytes = MPL_calloc((size_t) n, sizeof(*scounts_bytes), MPL_MEM_COLL);
        if (!scounts_bytes)
            return MPI_ERR_OTHER;
        scounts_bytes[rank] = recv_nbytes;
    }

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS) {
        MPL_free(scounts_bytes);
        MPL_free(sdispls_bytes);
        return MPI_ERR_OTHER;
    }

    st = cnc_scatterv_linear(sbuf_adj, scounts_bytes, sdispls_bytes, rbuf_adj, recv_nbytes,
                             n, rank, root, t);
    MPL_free(scounts_bytes);
    MPL_free(sdispls_bytes);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Scatterv_allcomm_linear(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                        recvtype, root, comm_ptr, coll_attr);
}

int MPIR_Allgather_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint recv_size, nbytes, send_lb = 0, recv_lb = 0;
    const void *sbuf_adj;
    void *rbuf_adj;

    if (!cnc_can_run(comm_ptr) || !cnc_contig_host(recvbuf, recvtype))
        goto fallback;

    MPIR_Datatype_get_size_macro(recvtype, recv_size);
    MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
    nbytes = recvcount * recv_size;
    rbuf_adj = (char *) recvbuf + recv_lb;

    if (sendbuf == MPI_IN_PLACE) {
        sbuf_adj = (const char *) rbuf_adj + comm_ptr->rank * nbytes;
    } else {
        MPI_Aint send_size;
        if (!cnc_contig_host(sendbuf, sendtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        if (sendcount * send_size != nbytes)
            goto fallback;
        MPIR_Datatype_get_true_lb(sendtype, &send_lb);
        sbuf_adj = (const char *) sendbuf + send_lb;
    }

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_allgather(sbuf_adj, rbuf_adj, nbytes, comm_ptr->local_size, comm_ptr->rank, t, NULL);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                     comm_ptr, coll_attr);
}

int MPIR_Allgatherv_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint recv_size, recv_extent, send_lb = 0, recv_lb = 0;
    const void *sbuf_adj;
    void *rbuf_adj;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    size_t *rcounts_bytes = NULL;
    ptrdiff_t *rdispls_bytes = NULL;

    if (!cnc_can_run(comm_ptr) || !cnc_contig_host(recvbuf, recvtype))
        goto fallback;

    MPIR_Datatype_get_size_macro(recvtype, recv_size);
    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Datatype_get_true_lb(recvtype, &recv_lb);
    rbuf_adj = (char *) recvbuf + recv_lb;

    for (int i = 0; i < n; i++) {
        if (recvcounts[i] < 0 || displs[i] * recv_extent < 0)
            goto fallback;
    }

    if (sendbuf == MPI_IN_PLACE) {
        sbuf_adj = (const char *) rbuf_adj + displs[rank] * recv_extent;
    } else {
        MPI_Aint send_size;
        if (!cnc_contig_host(sendbuf, sendtype))
            goto fallback;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        if (sendcount * send_size != recvcounts[rank] * recv_size)
            goto fallback;
        MPIR_Datatype_get_true_lb(sendtype, &send_lb);
        sbuf_adj = (const char *) sendbuf + send_lb;
    }

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;

    rcounts_bytes = MPL_malloc((size_t) n * sizeof(*rcounts_bytes), MPL_MEM_COLL);
    rdispls_bytes = MPL_malloc((size_t) n * sizeof(*rdispls_bytes), MPL_MEM_COLL);
    if (!rcounts_bytes || !rdispls_bytes) {
        MPL_free(rcounts_bytes);
        MPL_free(rdispls_bytes);
        return MPI_ERR_OTHER;
    }
    for (int i = 0; i < n; i++) {
        rcounts_bytes[i] = (size_t) (recvcounts[i] * recv_size);
        rdispls_bytes[i] = (ptrdiff_t) (displs[i] * recv_extent);
    }

    st = cnc_allgatherv_ring(sbuf_adj, rbuf_adj, rcounts_bytes, rdispls_bytes, n, rank, t);
    MPL_free(rcounts_bytes);
    MPL_free(rdispls_bytes);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Allgatherv_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                      recvtype, comm_ptr, coll_attr);
}

int MPIR_Alltoall_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                            MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint send_size, recv_size, nbytes, send_lb, recv_lb;

    if (sendbuf == MPI_IN_PLACE)
        return MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype,
                                                             recvbuf, recvcount, recvtype,
                                                             comm_ptr, coll_attr);

    if (!cnc_can_run(comm_ptr) || !cnc_contig_host(sendbuf, sendtype) ||
        !cnc_contig_host(recvbuf, recvtype))
        goto fallback;

    MPIR_Datatype_get_size_macro(sendtype, send_size);
    MPIR_Datatype_get_size_macro(recvtype, recv_size);
    nbytes = recvcount * recv_size;
    if (sendcount * send_size != nbytes)
        goto fallback;

    MPIR_Datatype_get_true_lb(sendtype, &send_lb);
    MPIR_Datatype_get_true_lb(recvtype, &recv_lb);

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_alltoall((const char *) sendbuf + send_lb, (char *) recvbuf + recv_lb, nbytes,
                      comm_ptr->local_size, comm_ptr->rank, t, NULL);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                        recvtype, comm_ptr, coll_attr);
}

int MPIR_Alltoallv_intra_cnc(const void *sendbuf, const MPI_Aint * sendcounts,
                             const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                             const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint send_size, recv_size, send_extent, recv_extent, send_lb, recv_lb;
    int n = comm_ptr->local_size;
    size_t *scounts_bytes = NULL, *rcounts_bytes = NULL;
    ptrdiff_t *sdispls_bytes = NULL, *rdispls_bytes = NULL;

    if (sendbuf == MPI_IN_PLACE)
        return MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                              sendtype, recvbuf, recvcounts,
                                                              rdispls, recvtype, comm_ptr,
                                                              coll_attr);

    if (!cnc_can_run(comm_ptr) || !cnc_contig_host(sendbuf, sendtype) ||
        !cnc_contig_host(recvbuf, recvtype))
        goto fallback;

    MPIR_Datatype_get_size_macro(sendtype, send_size);
    MPIR_Datatype_get_size_macro(recvtype, recv_size);
    MPIR_Datatype_get_extent_macro(sendtype, send_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Datatype_get_true_lb(sendtype, &send_lb);
    MPIR_Datatype_get_true_lb(recvtype, &recv_lb);

    for (int i = 0; i < n; i++) {
        if (sendcounts[i] < 0 || recvcounts[i] < 0 ||
            sdispls[i] * send_extent < 0 || rdispls[i] * recv_extent < 0)
            goto fallback;
    }
    if (sendcounts[comm_ptr->rank] * send_size != recvcounts[comm_ptr->rank] * recv_size)
        goto fallback;

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;

    scounts_bytes = MPL_malloc(2 * (size_t) n * sizeof(*scounts_bytes), MPL_MEM_COLL);
    sdispls_bytes = MPL_malloc(2 * (size_t) n * sizeof(*sdispls_bytes), MPL_MEM_COLL);
    if (!scounts_bytes || !sdispls_bytes) {
        MPL_free(scounts_bytes);
        MPL_free(sdispls_bytes);
        return MPI_ERR_OTHER;
    }
    rcounts_bytes = scounts_bytes + n;
    rdispls_bytes = sdispls_bytes + n;
    for (int i = 0; i < n; i++) {
        scounts_bytes[i] = (size_t) (sendcounts[i] * send_size);
        sdispls_bytes[i] = (ptrdiff_t) (sdispls[i] * send_extent);
        rcounts_bytes[i] = (size_t) (recvcounts[i] * recv_size);
        rdispls_bytes[i] = (ptrdiff_t) (rdispls[i] * recv_extent);
    }

    st = cnc_alltoallv_pairwise((const char *) sendbuf + send_lb, scounts_bytes, sdispls_bytes,
                                (char *) recvbuf + recv_lb, rcounts_bytes, rdispls_bytes,
                                n, comm_ptr->rank, t);
    MPL_free(scounts_bytes);
    MPL_free(sdispls_bytes);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                          recvcounts, rdispls, recvtype, comm_ptr, coll_attr);
}

int MPIR_Reduce_intra_cnc(const void *sendbuf, void *recvbuf, MPI_Aint count,
                          MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                          int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    cnc_reduce_opts_t opts;
    cnc_datatype_t dt;
    int rank = comm_ptr->rank;
    const void *sbuf_use;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    memset(&opts, 0, sizeof(opts));
    if (!cnc_map_dtype_op(datatype, op, &dt, &opts.op))
        goto fallback;

    if (rank == root && !cnc_contig_host(recvbuf, datatype))
        goto fallback;
    if (sendbuf != MPI_IN_PLACE && !cnc_contig_host(sendbuf, datatype))
        goto fallback;

    sbuf_use = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_reduce(sbuf_use, recvbuf, count, dt, comm_ptr->local_size, rank, root, t, &opts);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                      coll_attr);
}

int MPIR_Allreduce_intra_cnc(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    cnc_allreduce_opts_t opts;
    cnc_datatype_t dt;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    memset(&opts, 0, sizeof(opts));
    if (!cnc_map_dtype_op(datatype, op, &dt, &opts.op))
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

int MPIR_Reduce_scatter_block_intra_cnc(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                        int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    cnc_reduce_scatter_opts_t opts;
    cnc_datatype_t dt;

    if (!cnc_can_run(comm_ptr) || sendbuf == MPI_IN_PLACE)
        goto fallback;

    memset(&opts, 0, sizeof(opts));
    if (!cnc_map_dtype_op(datatype, op, &dt, &opts.op))
        goto fallback;

    if (!cnc_contig_host(sendbuf, datatype) || !cnc_contig_host(recvbuf, datatype))
        goto fallback;

    if (cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPI_ERR_OTHER;
    st = cnc_reduce_scatter(sendbuf, recvbuf, comm_ptr->local_size * recvcount,
                            dt, comm_ptr->local_size, comm_ptr->rank, t, &opts);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;

  fallback:
    return MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf, recvcount,
                                                              datatype, op, comm_ptr, coll_attr);
}
