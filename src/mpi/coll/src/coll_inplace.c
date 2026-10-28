/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "coll_csel.h"

/* The in-place adapter, for the algorithms with inplace_copy in coll_algorithms.txt. Their
 * generated wrapper calls MPIR_Coll_inplace_copy when sendbuf is MPI_IN_PLACE. The adapter
 * copies the receive buffer span to a temporary buffer, and calls the wrapper again with that
 * copy as the sendbuf and the receive side's counts and types as the send side. The algorithm
 * then runs as a not-in-place call and writes recvbuf directly, so there is no copy back.
 * A nonblocking call copies in the first step of its schedule, and the schedule owns the
 * temporary buffer. */

#define COLL_TYPE_INTRA_CASE(NAME) \
    case MPIR_CSEL_COLL_TYPE__INTRA_  ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTRA_I ## NAME

/* The bytes of the receive buffer that the algorithm reads and writes: [lo, hi) from buf */
struct recv_span {
    bool any;
    void *buf;
    MPI_Aint lo;
    MPI_Aint hi;
};

static void get_recv_span(MPIR_Csel_coll_sig_s * coll_sig, struct recv_span *sp);
static void add_block(struct recv_span *sp, MPI_Aint offset, MPI_Aint count, MPI_Datatype datatype);
static MPI_Aint get_extent(MPI_Datatype datatype);
static void set_send_side(MPIR_Csel_coll_sig_s * coll_sig, const void *sendbuf);

/* Calls fn with a copy of the receive span as the send side. fn is the generated wrapper of
 * the algorithm, which calls this again with sendbuf not MPI_IN_PLACE, so the algorithm runs. */
int MPIR_Coll_inplace_copy(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt,
                           MPIR_Coll_algo_fn fn)
{
    int mpi_errno = MPI_SUCCESS;
    struct recv_span sp;
    void *tmp = NULL;
    MPI_Aint size;

    /* device receive buffers: swap them for host buffers. The swap clears the gpu flag and
     * calls fn again, which comes back here for the host path below. */
    if (coll_sig->flags & MPIR_COLL_SIG_FLAG__GPU) {
        return MPIR_Coll_gpu_swap(coll_sig, cnt, fn);
    }

    /* the wrapper calls this before the schedule is created, so fn creates it */
    MPIR_Assert(coll_sig->sched == NULL);

    get_recv_span(coll_sig, &sp);
    size = sp.any ? sp.hi - sp.lo : 0;

    /* blocking coll_type is even */
    bool is_blocking = (coll_sig->coll_type % 2 == 0);

    /* saved so that the send side can be restored after fn */
    MPIR_Csel_coll_sig_s saved = *coll_sig;
    const void *sendbuf = sp.buf;
    if (size > 0) {
        tmp = MPL_malloc(size, MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!tmp, mpi_errno, MPI_ERR_OTHER, "**nomem");
        if (is_blocking) {
            /* MPIR_Localcopy takes the source from device memory too: alltoallw has no
             * gpu flag, so its receive buffer can be on the device here */
            mpi_errno = MPIR_Localcopy((char *) sp.buf + sp.lo, size, MPIR_BYTE_INTERNAL,
                                       tmp, size, MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);
        }
        /* tmp holds the byte at lo at its start, so the receive displacements address it
         * when sendbuf is shifted by -lo */
        sendbuf = (char *) tmp - sp.lo;
    }

    set_send_side(coll_sig, sendbuf);
    mpi_errno = fn(coll_sig, cnt);
    coll_sig->u = saved.u;
    MPIR_ERR_CHECK(mpi_errno);

    if (tmp && !is_blocking) {
        /* the schedule owns tmp, and the copy-in becomes its first step */
        if (coll_sig->sched_type == MPIR_SCHED_NORMAL) {
            MPIR_Sched_t s = coll_sig->sched;
            MPIR_Sched_add_buffer(s, tmp);
            tmp = NULL;

            mpi_errno = MPIR_Sched_copy((char *) sp.buf + sp.lo, size, MPIR_BYTE_INTERNAL,
                                        (char *) sendbuf + sp.lo, size, MPIR_BYTE_INTERNAL, s);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Sched_move_to_front(s, 1);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (coll_sig->sched_type == MPIR_SCHED_GENTRAN) {
            MPIR_TSP_sched_t s = coll_sig->sched;
            MPIR_TSP_sched_add_buffer(s, tmp);
            tmp = NULL;

            mpi_errno = MPIR_TSP_sched_localcopy_front((char *) sp.buf + sp.lo, size,
                                                       MPIR_BYTE_INTERNAL,
                                                       (char *) sendbuf + sp.lo, size,
                                                       MPIR_BYTE_INTERNAL, s);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_TSP_sched_fence(s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* MPIR_SCHED_REQUEST has no steps to add; nothing produces it for now */
            MPIR_Assert(0);
        }
    }

  fn_exit:
    MPL_free(tmp);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* The receive span of the intracomm alltoall family: each peer's block, from the block's
 * first byte to its last. Negative displacements and extents are allowed. */
static void get_recv_span(MPIR_Csel_coll_sig_s * coll_sig, struct recv_span *sp)
{
    MPIR_Comm *comm = coll_sig->comm_ptr;
    int size = comm->local_size;

    sp->any = false;
    sp->buf = NULL;
#define U(name) (&coll_sig->u.name)
    switch (coll_sig->coll_type) {
          COLL_TYPE_INTRA_CASE(ALLTOALL):
            sp->buf = U(alltoall)->recvbuf;
            for (int i = 0; i < size; i++) {
                MPI_Aint offset = i * U(alltoall)->recvcount * get_extent(U(alltoall)->recvtype);
                add_block(sp, offset, U(alltoall)->recvcount, U(alltoall)->recvtype);
            }
            break;
          COLL_TYPE_INTRA_CASE(ALLTOALLV):
            sp->buf = U(alltoallv)->recvbuf;
            for (int i = 0; i < size; i++) {
                MPI_Aint offset = U(alltoallv)->rdispls[i] * get_extent(U(alltoallv)->recvtype);
                add_block(sp, offset, U(alltoallv)->recvcounts[i], U(alltoallv)->recvtype);
            }
            break;
          COLL_TYPE_INTRA_CASE(ALLTOALLW):
            sp->buf = U(alltoallw)->recvbuf;
            for (int i = 0; i < size; i++) {
                /* alltoallw displacements are in bytes */
                add_block(sp, U(alltoallw)->rdispls[i], U(alltoallw)->recvcounts[i],
                          U(alltoallw)->recvtypes[i]);
            }
            break;
        default:
            MPIR_Assert(0);
    }
#undef U
}

/* Extends sp by count elements of datatype, starting at offset bytes from the buffer start */
static void add_block(struct recv_span *sp, MPI_Aint offset, MPI_Aint count, MPI_Datatype datatype)
{
    if (count == 0) {
        return;
    }
    MPI_Aint extent = get_extent(datatype);
    MPI_Aint true_lb, true_extent;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    /* the first and the last element; the extent may be negative */
    MPI_Aint first = offset;
    MPI_Aint last = offset + (count - 1) * extent;
    MPI_Aint lo = MPL_MIN(first, last) + true_lb;
    MPI_Aint hi = MPL_MAX(first, last) + true_lb + true_extent;

    if (!sp->any) {
        sp->any = true;
        sp->lo = lo;
        sp->hi = hi;
    } else {
        sp->lo = MPL_MIN(sp->lo, lo);
        sp->hi = MPL_MAX(sp->hi, hi);
    }
}

static MPI_Aint get_extent(MPI_Datatype datatype)
{
    MPI_Aint extent;
    MPIR_Datatype_get_extent_macro(datatype, extent);
    return extent;
}

/* Points the send side of coll_sig at the copy, with the receive side's counts and types. The
 * caller saves and restores coll_sig->u. */
static void set_send_side(MPIR_Csel_coll_sig_s * coll_sig, const void *sendbuf)
{
#define U(name) (&coll_sig->u.name)
    switch (coll_sig->coll_type) {
          COLL_TYPE_INTRA_CASE(ALLTOALL):
            U(alltoall)->sendbuf = sendbuf;
            U(alltoall)->sendcount = U(alltoall)->recvcount;
            U(alltoall)->sendtype = U(alltoall)->recvtype;
            break;
          COLL_TYPE_INTRA_CASE(ALLTOALLV):
            U(alltoallv)->sendbuf = sendbuf;
            U(alltoallv)->sendcounts = U(alltoallv)->recvcounts;
            U(alltoallv)->sdispls = U(alltoallv)->rdispls;
            U(alltoallv)->sendtype = U(alltoallv)->recvtype;
            break;
          COLL_TYPE_INTRA_CASE(ALLTOALLW):
            U(alltoallw)->sendbuf = sendbuf;
            U(alltoallw)->sendcounts = U(alltoallw)->recvcounts;
            U(alltoallw)->sdispls = U(alltoallw)->rdispls;
            U(alltoallw)->sendtypes = U(alltoallw)->recvtypes;
            break;
        default:
            MPIR_Assert(0);
    }
#undef U
}
