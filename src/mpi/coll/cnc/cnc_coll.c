/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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
    if (!priv) {
        comm_ptr->coll.cnc_priv = CNC_PRIV_SKIP;
        return MPI_ERR_OTHER;
    }

    /* MPII_Comm_copy allocates a dedicated context ID without copying user
     * communicator attributes or invoking attribute callbacks. */
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_ptr->local_size, NULL, &priv->dup_comm);
    if (mpi_errno != MPI_SUCCESS) {
        MPL_free(priv);
        comm_ptr->coll.cnc_priv = CNC_PRIV_SKIP;
        return mpi_errno;
    }

    priv->dup_comm->coll.cnc_priv = CNC_PRIV_SKIP;

    if (cnc_oob_mpi_create(&priv->oob, priv->dup_comm->handle) != CNC_OK) {
        MPIR_Comm_release(priv->dup_comm);
        MPL_free(priv);
        comm_ptr->coll.cnc_priv = CNC_PRIV_SKIP;
        return MPI_ERR_OTHER;
    }

    if (cnc_init(&priv->transport, &priv->oob) != CNC_OK) {
        cnc_oob_mpi_destroy(&priv->oob);
        MPIR_Comm_release(priv->dup_comm);
        MPL_free(priv);
        comm_ptr->coll.cnc_priv = CNC_PRIV_SKIP;
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

/* Stage a (buf, count, datatype) buffer of nbytes bytes for reading by
 * libcncoll. Contiguous host buffers are used in place; non-contiguous or
 * device buffers are packed into a temporary host buffer via MPIR_Localcopy
 * so ranks with differing local datatype layouts or memory kinds never
 * diverge on collective eligibility. */
static int cnc_stage_in(const void *buf, MPI_Aint count, MPI_Datatype datatype, size_t nbytes,
                        const void **wire_buf_out, void **tmp_out)
{
    MPI_Aint true_lb;

    *tmp_out = NULL;
    if (cnc_contig_host(buf, datatype)) {
        MPIR_Datatype_get_true_lb(datatype, &true_lb);
        *wire_buf_out = (const char *) buf + true_lb;
        return MPI_SUCCESS;
    }

    *tmp_out = MPL_malloc(nbytes ? nbytes : 1, MPL_MEM_COLL);
    if (!*tmp_out)
        return MPI_ERR_OTHER;
    if (nbytes > 0) {
        int mpi_errno = MPIR_Localcopy(buf, count, datatype, *tmp_out, (MPI_Aint) nbytes,
                                       MPIR_BYTE_INTERNAL);
        if (mpi_errno != MPI_SUCCESS) {
            MPL_free(*tmp_out);
            *tmp_out = NULL;
            return mpi_errno;
        }
    }
    *wire_buf_out = *tmp_out;
    return MPI_SUCCESS;
}

static int cnc_stage_out_alloc(void *buf, MPI_Datatype datatype, size_t nbytes,
                               void **wire_buf_out, void **tmp_out)
{
    MPI_Aint true_lb;

    *tmp_out = NULL;
    if (cnc_contig_host(buf, datatype)) {
        MPIR_Datatype_get_true_lb(datatype, &true_lb);
        *wire_buf_out = (char *) buf + true_lb;
        return MPI_SUCCESS;
    }

    *tmp_out = MPL_malloc(nbytes ? nbytes : 1, MPL_MEM_COLL);
    if (!*tmp_out)
        return MPI_ERR_OTHER;
    *wire_buf_out = *tmp_out;
    return MPI_SUCCESS;
}

static int cnc_stage_out_finish(void *user_buf, MPI_Aint count, MPI_Datatype datatype,
                                size_t nbytes, void *tmp_buf, int status_ok)
{
    int mpi_errno = MPI_SUCCESS;

    if (!tmp_buf)
        return status_ok ? MPI_SUCCESS : MPI_ERR_OTHER;
    if (status_ok && nbytes > 0)
        mpi_errno = MPIR_Localcopy(tmp_buf, (MPI_Aint) nbytes, MPIR_BYTE_INTERNAL, user_buf,
                                   count, datatype);
    MPL_free(tmp_buf);
    return status_ok ? mpi_errno : MPI_ERR_OTHER;
}

/* Prepare byte counts, byte displacements, and a contiguous host buffer for a
 * variable-count (*v) collective side. When the caller buffer is contiguous on
 * host and all non-empty displacements are non-negative, the user buffer is
 * used directly; otherwise segments are packed/unpacked into a contiguous
 * temporary buffer. */
static int cnc_v_stage_prep(const void *buf, const MPI_Aint * counts, const MPI_Aint * displs,
                            MPI_Datatype datatype, int n, int copy_in, int inplace_slot,
                            void **wire_buf_out, void **tmp_out, size_t * counts_bytes,
                            ptrdiff_t * displs_bytes)
{
    MPI_Aint type_size, type_extent, true_lb = 0;
    int direct;
    size_t total = 0;

    *tmp_out = NULL;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, type_extent);
    direct = cnc_contig_host(buf, datatype);
    for (int i = 0; i < n; i++) {
        size_t seg = counts[i] > 0 ? (size_t) (counts[i] * type_size) : 0;
        counts_bytes[i] = seg;
        if (counts[i] > 0 && displs[i] * type_extent < 0)
            direct = 0;
    }

    if (direct) {
        MPIR_Datatype_get_true_lb(datatype, &true_lb);
        for (int i = 0; i < n; i++)
            displs_bytes[i] = counts[i] > 0 ? (ptrdiff_t) (displs[i] * type_extent) : 0;
        *wire_buf_out = (char *) buf + true_lb;
        return MPI_SUCCESS;
    }

    for (int i = 0; i < n; i++) {
        displs_bytes[i] = (ptrdiff_t) total;
        total += counts_bytes[i];
    }
    *tmp_out = MPL_malloc(total ? total : 1, MPL_MEM_COLL);
    if (!*tmp_out)
        return MPI_ERR_OTHER;
    if (copy_in) {
        for (int i = 0; i < n; i++) {
            if (counts_bytes[i] > 0) {
                int rc = MPIR_Localcopy((const char *) buf + displs[i] * type_extent, counts[i],
                                        datatype, (char *) *tmp_out + displs_bytes[i],
                                        (MPI_Aint) counts_bytes[i], MPIR_BYTE_INTERNAL);
                if (rc != MPI_SUCCESS) {
                    MPL_free(*tmp_out);
                    *tmp_out = NULL;
                    return rc;
                }
            }
        }
    } else if (inplace_slot >= 0 && counts_bytes[inplace_slot] > 0) {
        int rc = MPIR_Localcopy((const char *) buf + displs[inplace_slot] * type_extent,
                                counts[inplace_slot], datatype,
                                (char *) *tmp_out + displs_bytes[inplace_slot],
                                (MPI_Aint) counts_bytes[inplace_slot], MPIR_BYTE_INTERNAL);
        if (rc != MPI_SUCCESS) {
            MPL_free(*tmp_out);
            *tmp_out = NULL;
            return rc;
        }
    }
    *wire_buf_out = *tmp_out;
    return MPI_SUCCESS;
}

static int cnc_v_stage_unpack(void *user_buf, const MPI_Aint * counts, const MPI_Aint * displs,
                              MPI_Datatype datatype, int n, const size_t * counts_bytes,
                              const ptrdiff_t * displs_bytes, void *tmp_buf, int status_ok)
{
    MPI_Aint type_extent;
    int mpi_errno = MPI_SUCCESS;

    if (!tmp_buf)
        return status_ok ? MPI_SUCCESS : MPI_ERR_OTHER;
    if (status_ok) {
        MPIR_Datatype_get_extent_macro(datatype, type_extent);
        for (int i = 0; i < n; i++) {
            if (counts_bytes[i] > 0) {
                mpi_errno = MPIR_Localcopy((const char *) tmp_buf + displs_bytes[i],
                                           (MPI_Aint) counts_bytes[i], MPIR_BYTE_INTERNAL,
                                           (char *) user_buf + displs[i] * type_extent,
                                           counts[i], datatype);
                if (mpi_errno != MPI_SUCCESS)
                    break;
            }
        }
    }
    MPL_free(tmp_buf);
    return status_ok ? mpi_errno : MPI_ERR_OTHER;
}

static int cnc_map_dtype_op(MPI_Datatype datatype, MPI_Op op, cnc_datatype_t * dt_out,
                            cnc_reduce_op_t * op_out)
{
    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(datatype)) {
        case MPIR_FLOAT32:
            *dt_out = CNC_DTYPE_F32;
            break;
        case MPIR_INT32:
            *dt_out = CNC_DTYPE_I32;
            break;
        default:
            return 0;
    }

    switch (op) {
        case MPI_SUM:
            *op_out = CNC_REDUCE_SUM;
            break;
        case MPI_PROD:
            *op_out = CNC_REDUCE_PROD;
            break;
        case MPI_MIN:
            *op_out = CNC_REDUCE_MIN;
            break;
        case MPI_MAX:
            *op_out = CNC_REDUCE_MAX;
            break;
        default:
            return 0;
    }

    return 1;
}

int MPIR_Barrier_intra_cnc(MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Barrier_intra_dissemination(comm_ptr, coll_attr);

    cnc_barrier(comm_ptr->local_size, comm_ptr->rank, t);
    return MPI_SUCCESS;
}

int MPIR_Bcast_intra_cnc(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint type_size;
    size_t nbytes;
    void *wire_buf = NULL, *tmp_buf = NULL;
    int rank = comm_ptr->rank;
    int rc;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, coll_attr);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = (size_t) (count * type_size);

    if (rank == root) {
        const void *sbuf_wire = NULL;
        rc = cnc_stage_in(buffer, count, datatype, nbytes, &sbuf_wire, &tmp_buf);
        wire_buf = (void *) sbuf_wire;
    } else {
        rc = cnc_stage_out_alloc(buffer, datatype, nbytes, &wire_buf, &tmp_buf);
    }
    if (rc != MPI_SUCCESS)
        return rc;

    st = cnc_bcast(CNC_IN_PLACE, wire_buf, nbytes, comm_ptr->local_size, rank, root, t, NULL);
    if (rank == root) {
        MPL_free(tmp_buf);
        return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;
    }
    return cnc_stage_out_finish(buffer, count, datatype, nbytes, tmp_buf, st == CNC_OK);
}

int MPIR_Gather_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                          void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                          MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    size_t nbytes;
    const void *sbuf_adj = NULL;
    void *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    int rc;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, coll_attr);

    if (rank == root) {
        MPI_Aint recv_size, recv_extent;
        MPIR_Datatype_get_size_macro(recvtype, recv_size);
        nbytes = (size_t) (recvcount * recv_size);
        rc = cnc_stage_out_alloc(recvbuf, recvtype, (size_t) n * nbytes, &rbuf_adj, &tmp_rbuf);
        if (rc != MPI_SUCCESS)
            return rc;
        if (sendbuf == MPI_IN_PLACE) {
            sbuf_adj = (const char *) rbuf_adj + rank * nbytes;
            if (tmp_rbuf && nbytes > 0) {
                MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
                rc = MPIR_Localcopy((const char *) recvbuf + rank * recvcount * recv_extent,
                                    recvcount, recvtype, (void *) sbuf_adj, (MPI_Aint) nbytes,
                                    MPIR_BYTE_INTERNAL);
                if (rc != MPI_SUCCESS) {
                    MPL_free(tmp_rbuf);
                    return rc;
                }
            }
        } else {
            rc = cnc_stage_in(sendbuf, sendcount, sendtype, nbytes, &sbuf_adj, &tmp_sbuf);
            if (rc != MPI_SUCCESS) {
                MPL_free(tmp_rbuf);
                return rc;
            }
        }
    } else {
        MPI_Aint send_size;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        nbytes = (size_t) (sendcount * send_size);
        rc = cnc_stage_in(sendbuf, sendcount, sendtype, nbytes, &sbuf_adj, &tmp_sbuf);
        if (rc != MPI_SUCCESS)
            return rc;
    }

    st = cnc_gather(sbuf_adj, rbuf_adj, nbytes, n, rank, root, t, NULL);
    MPL_free(tmp_sbuf);
    if (rank == root)
        return cnc_stage_out_finish(recvbuf, recvcount * (MPI_Aint) n, recvtype,
                                    (size_t) n * nbytes, tmp_rbuf, st == CNC_OK);
    return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;
}

int MPIR_Gatherv_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                           MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    size_t send_nbytes;
    const void *sbuf_adj = NULL;
    void *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    size_t *rcounts_bytes = NULL;
    ptrdiff_t *rdispls_bytes = NULL;
    int rc;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, root, comm_ptr, coll_attr);

    if (rank == root) {
        rcounts_bytes = MPL_malloc((size_t) n * sizeof(*rcounts_bytes), MPL_MEM_COLL);
        rdispls_bytes = MPL_malloc((size_t) n * sizeof(*rdispls_bytes), MPL_MEM_COLL);
        if (!rcounts_bytes || !rdispls_bytes) {
            MPL_free(rcounts_bytes);
            MPL_free(rdispls_bytes);
            return MPI_ERR_OTHER;
        }
        rc = cnc_v_stage_prep(recvbuf, recvcounts, displs, recvtype, n, 0,
                              sendbuf == MPI_IN_PLACE ? rank : -1, &rbuf_adj, &tmp_rbuf,
                              rcounts_bytes, rdispls_bytes);
        if (rc != MPI_SUCCESS) {
            MPL_free(rcounts_bytes);
            MPL_free(rdispls_bytes);
            return rc;
        }
        send_nbytes = rcounts_bytes[rank];
        if (sendbuf == MPI_IN_PLACE) {
            sbuf_adj = (const char *) rbuf_adj + rdispls_bytes[rank];
        } else {
            rc = cnc_stage_in(sendbuf, sendcount, sendtype, send_nbytes, &sbuf_adj, &tmp_sbuf);
            if (rc != MPI_SUCCESS) {
                MPL_free(tmp_rbuf);
                MPL_free(rcounts_bytes);
                MPL_free(rdispls_bytes);
                return rc;
            }
        }
    } else {
        MPI_Aint send_size;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        send_nbytes = sendcount > 0 ? (size_t) (sendcount * send_size) : 0;
        rc = cnc_stage_in(sendbuf, sendcount, sendtype, send_nbytes, &sbuf_adj, &tmp_sbuf);
        if (rc != MPI_SUCCESS)
            return rc;
        rcounts_bytes = MPL_calloc((size_t) n, sizeof(*rcounts_bytes), MPL_MEM_COLL);
        if (!rcounts_bytes) {
            MPL_free(tmp_sbuf);
            return MPI_ERR_OTHER;
        }
        rcounts_bytes[rank] = send_nbytes;
    }

    st = cnc_gatherv_linear(sbuf_adj, send_nbytes, rbuf_adj, rcounts_bytes, rdispls_bytes,
                            n, rank, root, t);
    MPL_free(tmp_sbuf);
    if (rank == root)
        rc = cnc_v_stage_unpack(recvbuf, recvcounts, displs, recvtype, n, rcounts_bytes,
                                rdispls_bytes, tmp_rbuf, st == CNC_OK);
    else
        rc = (st == CNC_OK) ? MPI_SUCCESS : MPI_ERR_OTHER;
    MPL_free(rcounts_bytes);
    MPL_free(rdispls_bytes);
    return rc;
}

int MPIR_Scatter_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                           void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                           MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    size_t nbytes;
    const void *sbuf_adj = NULL;
    void *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    int rc;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, coll_attr);

    if (rank == root) {
        MPI_Aint send_size;
        MPIR_Datatype_get_size_macro(sendtype, send_size);
        nbytes = (size_t) (sendcount * send_size);
        rc = cnc_stage_in(sendbuf, sendcount * (MPI_Aint) n, sendtype, (size_t) n * nbytes,
                          &sbuf_adj, &tmp_sbuf);
        if (rc != MPI_SUCCESS)
            return rc;
        if (recvbuf == MPI_IN_PLACE) {
            rbuf_adj = CNC_IN_PLACE;
        } else {
            rc = cnc_stage_out_alloc(recvbuf, recvtype, nbytes, &rbuf_adj, &tmp_rbuf);
            if (rc != MPI_SUCCESS) {
                MPL_free(tmp_sbuf);
                return rc;
            }
        }
    } else {
        MPI_Aint recv_size;
        MPIR_Datatype_get_size_macro(recvtype, recv_size);
        nbytes = (size_t) (recvcount * recv_size);
        rc = cnc_stage_out_alloc(recvbuf, recvtype, nbytes, &rbuf_adj, &tmp_rbuf);
        if (rc != MPI_SUCCESS)
            return rc;
    }

    st = cnc_scatter(sbuf_adj, rbuf_adj, nbytes, n, rank, root, t, NULL);
    MPL_free(tmp_sbuf);
    if (rank == root && recvbuf == MPI_IN_PLACE)
        return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;
    return cnc_stage_out_finish(recvbuf, recvcount, recvtype, nbytes, tmp_rbuf, st == CNC_OK);
}

int MPIR_Scatterv_intra_cnc(const void *sendbuf, const MPI_Aint * sendcounts,
                            const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                            MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    size_t recv_nbytes;
    void *sbuf_adj = NULL, *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    size_t *scounts_bytes = NULL;
    ptrdiff_t *sdispls_bytes = NULL;
    int rc;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Scatterv_allcomm_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, coll_attr);

    if (rank == root) {
        scounts_bytes = MPL_malloc((size_t) n * sizeof(*scounts_bytes), MPL_MEM_COLL);
        sdispls_bytes = MPL_malloc((size_t) n * sizeof(*sdispls_bytes), MPL_MEM_COLL);
        if (!scounts_bytes || !sdispls_bytes) {
            MPL_free(scounts_bytes);
            MPL_free(sdispls_bytes);
            return MPI_ERR_OTHER;
        }
        rc = cnc_v_stage_prep(sendbuf, sendcounts, displs, sendtype, n, 1, -1, &sbuf_adj,
                              &tmp_sbuf, scounts_bytes, sdispls_bytes);
        if (rc != MPI_SUCCESS) {
            MPL_free(scounts_bytes);
            MPL_free(sdispls_bytes);
            return rc;
        }
        recv_nbytes = scounts_bytes[rank];
        if (recvbuf == MPI_IN_PLACE) {
            rbuf_adj = (char *) sbuf_adj + sdispls_bytes[rank];
        } else {
            rc = cnc_stage_out_alloc(recvbuf, recvtype, recv_nbytes, &rbuf_adj, &tmp_rbuf);
            if (rc != MPI_SUCCESS) {
                MPL_free(tmp_sbuf);
                MPL_free(scounts_bytes);
                MPL_free(sdispls_bytes);
                return rc;
            }
        }
    } else {
        MPI_Aint recv_size;
        MPIR_Datatype_get_size_macro(recvtype, recv_size);
        recv_nbytes = recvcount > 0 ? (size_t) (recvcount * recv_size) : 0;
        rc = cnc_stage_out_alloc(recvbuf, recvtype, recv_nbytes, &rbuf_adj, &tmp_rbuf);
        if (rc != MPI_SUCCESS)
            return rc;
        scounts_bytes = MPL_calloc((size_t) n, sizeof(*scounts_bytes), MPL_MEM_COLL);
        if (!scounts_bytes) {
            MPL_free(tmp_rbuf);
            return MPI_ERR_OTHER;
        }
        scounts_bytes[rank] = recv_nbytes;
    }

    st = cnc_scatterv_linear(sbuf_adj, scounts_bytes, sdispls_bytes, rbuf_adj, recv_nbytes,
                             n, rank, root, t);
    MPL_free(tmp_sbuf);
    MPL_free(scounts_bytes);
    MPL_free(sdispls_bytes);
    if (rank == root && recvbuf == MPI_IN_PLACE)
        return st == CNC_OK ? MPI_SUCCESS : MPI_ERR_OTHER;
    return cnc_stage_out_finish(recvbuf, recvcount, recvtype, recv_nbytes, tmp_rbuf, st == CNC_OK);
}

int MPIR_Allgather_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint recv_size, recv_extent;
    size_t nbytes;
    const void *sbuf_adj = NULL;
    void *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    int rc;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                         recvtype, comm_ptr, coll_attr);

    MPIR_Datatype_get_size_macro(recvtype, recv_size);
    nbytes = (size_t) (recvcount * recv_size);
    rc = cnc_stage_out_alloc(recvbuf, recvtype, (size_t) n * nbytes, &rbuf_adj, &tmp_rbuf);
    if (rc != MPI_SUCCESS)
        return rc;

    if (sendbuf == MPI_IN_PLACE) {
        sbuf_adj = (const char *) rbuf_adj + rank * nbytes;
        if (tmp_rbuf && nbytes > 0) {
            MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
            rc = MPIR_Localcopy((const char *) recvbuf + rank * recvcount * recv_extent,
                                recvcount, recvtype, (void *) sbuf_adj, (MPI_Aint) nbytes,
                                MPIR_BYTE_INTERNAL);
            if (rc != MPI_SUCCESS) {
                MPL_free(tmp_rbuf);
                return rc;
            }
        }
    } else {
        rc = cnc_stage_in(sendbuf, sendcount, sendtype, nbytes, &sbuf_adj, &tmp_sbuf);
        if (rc != MPI_SUCCESS) {
            MPL_free(tmp_rbuf);
            return rc;
        }
    }

    st = cnc_allgather(sbuf_adj, rbuf_adj, nbytes, n, rank, t, NULL);
    MPL_free(tmp_sbuf);
    return cnc_stage_out_finish(recvbuf, recvcount * (MPI_Aint) n, recvtype,
                                (size_t) n * nbytes, tmp_rbuf, st == CNC_OK);
}

int MPIR_Allgatherv_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    const void *sbuf_adj = NULL;
    void *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    size_t *rcounts_bytes = NULL;
    ptrdiff_t *rdispls_bytes = NULL;
    int rc;

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Allgatherv_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                          displs, recvtype, comm_ptr, coll_attr);

    rcounts_bytes = MPL_malloc((size_t) n * sizeof(*rcounts_bytes), MPL_MEM_COLL);
    rdispls_bytes = MPL_malloc((size_t) n * sizeof(*rdispls_bytes), MPL_MEM_COLL);
    if (!rcounts_bytes || !rdispls_bytes) {
        MPL_free(rcounts_bytes);
        MPL_free(rdispls_bytes);
        return MPI_ERR_OTHER;
    }

    rc = cnc_v_stage_prep(recvbuf, recvcounts, displs, recvtype, n, 0,
                          sendbuf == MPI_IN_PLACE ? rank : -1, &rbuf_adj, &tmp_rbuf,
                          rcounts_bytes, rdispls_bytes);
    if (rc != MPI_SUCCESS) {
        MPL_free(rcounts_bytes);
        MPL_free(rdispls_bytes);
        return rc;
    }

    if (sendbuf == MPI_IN_PLACE) {
        sbuf_adj = (const char *) rbuf_adj + rdispls_bytes[rank];
    } else {
        rc = cnc_stage_in(sendbuf, sendcount, sendtype, rcounts_bytes[rank], &sbuf_adj, &tmp_sbuf);
        if (rc != MPI_SUCCESS) {
            MPL_free(tmp_rbuf);
            MPL_free(rcounts_bytes);
            MPL_free(rdispls_bytes);
            return rc;
        }
    }

    st = cnc_allgatherv_ring(sbuf_adj, rbuf_adj, rcounts_bytes, rdispls_bytes, n, rank, t);
    MPL_free(tmp_sbuf);
    rc = cnc_v_stage_unpack(recvbuf, recvcounts, displs, recvtype, n, rcounts_bytes,
                            rdispls_bytes, tmp_rbuf, st == CNC_OK);
    MPL_free(rcounts_bytes);
    MPL_free(rdispls_bytes);
    return rc;
}

int MPIR_Alltoall_intra_cnc(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                            MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    MPI_Aint recv_size;
    size_t nbytes, total_bytes;
    const void *sbuf_adj = NULL;
    void *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    int rc;

    if (sendbuf == MPI_IN_PLACE)
        return MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype,
                                                             recvbuf, recvcount, recvtype,
                                                             comm_ptr, coll_attr);

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm_ptr, coll_attr);

    MPIR_Datatype_get_size_macro(recvtype, recv_size);
    nbytes = (size_t) (recvcount * recv_size);
    total_bytes = n * nbytes;

    rc = cnc_stage_in(sendbuf, sendcount * (MPI_Aint) n, sendtype, total_bytes, &sbuf_adj,
                      &tmp_sbuf);
    if (rc != MPI_SUCCESS)
        return rc;
    rc = cnc_stage_out_alloc(recvbuf, recvtype, total_bytes, &rbuf_adj, &tmp_rbuf);
    if (rc != MPI_SUCCESS) {
        MPL_free(tmp_sbuf);
        return rc;
    }

    st = cnc_alltoall(sbuf_adj, rbuf_adj, nbytes, n, comm_ptr->rank, t, NULL);
    MPL_free(tmp_sbuf);
    return cnc_stage_out_finish(recvbuf, recvcount * (MPI_Aint) n, recvtype, total_bytes,
                                tmp_rbuf, st == CNC_OK);
}

int MPIR_Alltoallv_intra_cnc(const void *sendbuf, const MPI_Aint * sendcounts,
                             const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                             const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr, int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    void *sbuf_adj = NULL, *rbuf_adj = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int n = comm_ptr->local_size;
    size_t *scounts_bytes = NULL, *rcounts_bytes = NULL;
    ptrdiff_t *sdispls_bytes = NULL, *rdispls_bytes = NULL;
    int rc;

    if (sendbuf == MPI_IN_PLACE)
        return MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                              sendtype, recvbuf, recvcounts,
                                                              rdispls, recvtype, comm_ptr,
                                                              coll_attr);

    if (!cnc_can_run(comm_ptr) || cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        return MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                              recvcounts, rdispls, recvtype, comm_ptr, coll_attr);

    scounts_bytes = MPL_malloc(2 * (size_t) n * sizeof(*scounts_bytes), MPL_MEM_COLL);
    sdispls_bytes = MPL_malloc(2 * (size_t) n * sizeof(*sdispls_bytes), MPL_MEM_COLL);
    if (!scounts_bytes || !sdispls_bytes) {
        MPL_free(scounts_bytes);
        MPL_free(sdispls_bytes);
        return MPI_ERR_OTHER;
    }
    rcounts_bytes = scounts_bytes + n;
    rdispls_bytes = sdispls_bytes + n;

    rc = cnc_v_stage_prep(sendbuf, sendcounts, sdispls, sendtype, n, 1, -1, &sbuf_adj, &tmp_sbuf,
                          scounts_bytes, sdispls_bytes);
    if (rc != MPI_SUCCESS) {
        MPL_free(scounts_bytes);
        MPL_free(sdispls_bytes);
        return rc;
    }
    rc = cnc_v_stage_prep(recvbuf, recvcounts, rdispls, recvtype, n, 0, -1, &rbuf_adj, &tmp_rbuf,
                          rcounts_bytes, rdispls_bytes);
    if (rc != MPI_SUCCESS) {
        MPL_free(tmp_sbuf);
        MPL_free(scounts_bytes);
        MPL_free(sdispls_bytes);
        return rc;
    }

    st = cnc_alltoallv_pairwise(sbuf_adj, scounts_bytes, sdispls_bytes, rbuf_adj, rcounts_bytes,
                                rdispls_bytes, n, comm_ptr->rank, t);
    MPL_free(tmp_sbuf);
    rc = cnc_v_stage_unpack(recvbuf, recvcounts, rdispls, recvtype, n, rcounts_bytes,
                            rdispls_bytes, tmp_rbuf, st == CNC_OK);
    MPL_free(scounts_bytes);
    MPL_free(sdispls_bytes);
    return rc;
}

int MPIR_Reduce_intra_cnc(const void *sendbuf, void *recvbuf, MPI_Aint count,
                          MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                          int coll_attr)
{
    cnc_transport_t *t;
    cnc_status_t st;
    cnc_reduce_opts_t opts;
    cnc_datatype_t dt;
    MPI_Aint type_size;
    size_t nbytes;
    int rank = comm_ptr->rank;
    const void *sbuf_use = NULL;
    void *rbuf_use = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int rc;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    memset(&opts, 0, sizeof(opts));
    if (!cnc_map_dtype_op(datatype, op, &dt, &opts.op) ||
        cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        goto fallback;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = (size_t) (count * type_size);

    if (rank == root) {
        if (sendbuf == MPI_IN_PLACE) {
            rc = cnc_stage_in(recvbuf, count, datatype, nbytes, &sbuf_use, &tmp_rbuf);
            if (rc != MPI_SUCCESS)
                return rc;
            rbuf_use = (void *) sbuf_use;
        } else {
            rc = cnc_stage_in(sendbuf, count, datatype, nbytes, &sbuf_use, &tmp_sbuf);
            if (rc != MPI_SUCCESS)
                return rc;
            rc = cnc_stage_out_alloc(recvbuf, datatype, nbytes, &rbuf_use, &tmp_rbuf);
            if (rc != MPI_SUCCESS) {
                MPL_free(tmp_sbuf);
                return rc;
            }
        }
    } else {
        rc = cnc_stage_in(sendbuf, count, datatype, nbytes, &sbuf_use, &tmp_sbuf);
        if (rc != MPI_SUCCESS)
            return rc;
    }

    st = cnc_reduce(sbuf_use, rbuf_use, count, dt, comm_ptr->local_size, rank, root, t,
                    opts.op == CNC_REDUCE_SUM ? NULL : &opts);
    MPL_free(tmp_sbuf);
    if (rank == root)
        return cnc_stage_out_finish(recvbuf, count, datatype, nbytes, tmp_rbuf, st == CNC_OK);
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
    MPI_Aint type_size;
    size_t nbytes;
    const void *sbuf_use = NULL;
    void *rbuf_use = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int rc;

    if (!cnc_can_run(comm_ptr))
        goto fallback;

    memset(&opts, 0, sizeof(opts));
    if (!cnc_map_dtype_op(datatype, op, &dt, &opts.op) ||
        cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        goto fallback;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = (size_t) (count * type_size);

    if (sendbuf == MPI_IN_PLACE) {
        const void *in_wire = NULL;
        rc = cnc_stage_in(recvbuf, count, datatype, nbytes, &in_wire, &tmp_rbuf);
        if (rc != MPI_SUCCESS)
            return rc;
        rbuf_use = (void *) in_wire;
        sbuf_use = CNC_IN_PLACE;
    } else {
        rc = cnc_stage_in(sendbuf, count, datatype, nbytes, &sbuf_use, &tmp_sbuf);
        if (rc != MPI_SUCCESS)
            return rc;
        rc = cnc_stage_out_alloc(recvbuf, datatype, nbytes, &rbuf_use, &tmp_rbuf);
        if (rc != MPI_SUCCESS) {
            MPL_free(tmp_sbuf);
            return rc;
        }
    }

    st = cnc_allreduce(sbuf_use, rbuf_use, count, dt, comm_ptr->local_size, comm_ptr->rank, t,
                       opts.op == CNC_REDUCE_SUM ? NULL : &opts);
    MPL_free(tmp_sbuf);
    return cnc_stage_out_finish(recvbuf, count, datatype, nbytes, tmp_rbuf, st == CNC_OK);

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
    MPI_Aint type_size;
    size_t recv_nbytes, total_nbytes;
    int n = comm_ptr->local_size;
    const void *sbuf_use = NULL;
    void *rbuf_use = NULL, *tmp_sbuf = NULL, *tmp_rbuf = NULL;
    int rc;

    if (!cnc_can_run(comm_ptr) || sendbuf == MPI_IN_PLACE)
        goto fallback;

    memset(&opts, 0, sizeof(opts));
    if (!cnc_map_dtype_op(datatype, op, &dt, &opts.op) ||
        cnc_get_transport(comm_ptr, &t) != MPI_SUCCESS)
        goto fallback;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    recv_nbytes = (size_t) (recvcount * type_size);
    total_nbytes = n * recv_nbytes;

    rc = cnc_stage_in(sendbuf, recvcount * (MPI_Aint) n, datatype, total_nbytes, &sbuf_use,
                      &tmp_sbuf);
    if (rc != MPI_SUCCESS)
        return rc;
    rc = cnc_stage_out_alloc(recvbuf, datatype, recv_nbytes, &rbuf_use, &tmp_rbuf);
    if (rc != MPI_SUCCESS) {
        MPL_free(tmp_sbuf);
        return rc;
    }

    st = cnc_reduce_scatter(sbuf_use, rbuf_use, (size_t) n * (size_t) recvcount, dt, n,
                            comm_ptr->rank, t, opts.op == CNC_REDUCE_SUM ? NULL : &opts);
    MPL_free(tmp_sbuf);
    return cnc_stage_out_finish(recvbuf, recvcount, datatype, recv_nbytes, tmp_rbuf, st == CNC_OK);

  fallback:
    return MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf, recvcount,
                                                              datatype, op, comm_ptr, coll_attr);
}
