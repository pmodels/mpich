/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "coll_csel.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ENABLE_GPU_COLL
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, the user asserts that within each collective call, either every rank
        passes host buffers or every rank passes device buffers. It lets the collective
        selection use algorithms that need device buffers on all ranks, such as GPU IPC
        algorithms (the gpu_coll restriction). MPICH does not check the assertion.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* The buffers of a collective call that this rank uses, for the gpu restrictions and for
 * buffer swapping. Each entry is a user buffer as a (count, datatype) span, and whether the
 * collective reads it, writes it, or both. A buffer whose span has gaps between the blocks
 * (e.g. the recvbuf of allgatherv) is marked as read too, so that copying the span back does
 * not overwrite the gaps. */

#define COLL_BUF_READ  0x1
#define COLL_BUF_WRITE 0x2
#define COLL_BUF_RW    (COLL_BUF_READ | COLL_BUF_WRITE)

#define MAX_COLL_BUFS 2

struct coll_buf {
    void **field;               /* the buffer field in coll_sig */
    MPI_Aint count;
    MPI_Datatype datatype;
    int dir;
};

struct coll_bufs {
    int n;
    struct coll_buf bufs[MAX_COLL_BUFS];
};

#define COLL_TYPE_INTRA_CASE(NAME) \
    case MPIR_CSEL_COLL_TYPE__INTRA_  ## NAME: \
    case MPIR_CSEL_COLL_TYPE__INTRA_I ## NAME

/* a swapped buffer: the user buffer, the host buffer, and the host buffer as allocated */
struct swap_buf {
    void *user;
    void *host;
    void *raw;
};

static int run_auto(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt);
static int swap_and_run(MPIR_Csel_coll_sig_s * coll_sig, MPIR_Coll_algo_fn fn,
                        MPII_Csel_container_s * cnt);
static bool get_coll_bufs(MPIR_Csel_coll_sig_s * coll_sig, struct coll_bufs *bufs);
static int add_swap_steps(MPIR_Csel_coll_sig_s * coll_sig, struct coll_bufs *bufs,
                          struct swap_buf *swaps);
static void *alloc_host_buf(MPI_Aint count, MPI_Datatype datatype, void **raw);
static void add_send_recv(struct coll_bufs *bufs, const void **sendbuf, MPI_Aint sendcount,
                          MPI_Datatype sendtype, void **recvbuf, MPI_Aint recvcount,
                          MPI_Datatype recvtype, bool has_recv);
static void add_buf(struct coll_bufs *bufs, const void **field, MPI_Aint count,
                    MPI_Datatype datatype, int dir);
static MPI_Aint get_span(const MPI_Aint * counts, const MPI_Aint * displs, int n);

void MPIR_Coll_sig_prepare_gpu(MPIR_Csel_coll_sig_s * coll_sig)
{
    if (!ENABLE_GPU) {
        return;
    }

    struct coll_bufs bufs;
    if (!get_coll_bufs(coll_sig, &bufs) || bufs.n == 0) {
        return;
    }

    bool any_dev = false;
    bool all_dev = true;
    for (int i = 0; i < bufs.n; i++) {
        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(*bufs.bufs[i].field, &attr);
        if (MPL_gpu_attr_is_dev(&attr)) {
            any_dev = true;
        } else {
            all_dev = false;
        }
    }

    if (any_dev) {
        coll_sig->flags |= MPIR_COLL_SIG_FLAG__GPU;
    }
    if (all_dev && MPIR_CVAR_ENABLE_GPU_COLL) {
        coll_sig->flags |= MPIR_COLL_SIG_FLAG__GPU_COLL;
    }
}

/* General algorithm: swap the device buffers for host buffers, and continue in the auto
 * tree. The swap clears the gpu flags, so the auto tree cannot select
 * buffer_swap again. */
int MPIR_Coll_buffer_swap(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * me)
{
    int mpi_errno = MPI_SUCCESS;

    /* a tree that selects buffer_swap where gpu does not hold would loop */
    MPIR_ERR_CHKANDJUMP(!(coll_sig->flags & MPIR_COLL_SIG_FLAG__GPU), mpi_errno, MPI_ERR_OTHER,
                        "**nosupport");

    mpi_errno = swap_and_run(coll_sig, run_auto, me);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* For the algorithms with gpu_swap in coll_algorithms.txt, which can't work on device buffers:
 * their generated wrapper calls this when the gpu flag is set, with itself as fn. The swap
 * clears the flag, so fn then runs the algorithm on the host buffers. */
int MPIR_Coll_gpu_swap(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt,
                       MPIR_Coll_algo_fn fn)
{
    MPIR_Assert(coll_sig->flags & MPIR_COLL_SIG_FLAG__GPU);
    return swap_and_run(coll_sig, fn, cnt);
}

static int run_auto(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    return MPIR_Coll_run_auto(coll_sig);
}

/* Swaps the device buffers for host buffers, clears the gpu flags, and calls
 * fn, which either selects (run_auto) or is an algorithm.
 *
 * A blocking collective copies in, runs, and copies out inline. For nonblocking and persistent
 * collectives, fn only builds the schedule, so the copies are added to the schedule before
 * it starts: the copy-in at the front and the copy-out at the back. A persistent schedule
 * replays them on every start. The schedule owns the host buffers and frees them with
 * itself. */
static int swap_and_run(MPIR_Csel_coll_sig_s * coll_sig, MPIR_Coll_algo_fn fn,
                        MPII_Csel_container_s * cnt)
{
    int mpi_errno = MPI_SUCCESS;
    struct coll_bufs bufs;
    struct swap_buf swaps[MAX_COLL_BUFS];

    bufs.n = 0;
    memset(swaps, 0, sizeof(swaps));

    coll_sig->flags &= ~(MPIR_COLL_SIG_FLAG__GPU | MPIR_COLL_SIG_FLAG__GPU_COLL);

    /* blocking coll_type is even */
    bool is_blocking = (coll_sig->coll_type % 2 == 0);
    /* Nothing selects into an existing schedule yet: the nonblocking compositions are
     * disabled. Swapping inside one would need the copies around the sub-call's steps only. */
    MPIR_Assert(coll_sig->sched == NULL);

    /* the gpu flag is only set for the collectives get_coll_bufs describes */
    MPIR_Assertp(get_coll_bufs(coll_sig, &bufs));

    for (int i = 0; i < bufs.n; i++) {
        struct coll_buf *b = &bufs.bufs[i];
        if (!MPIR_GPU_query_pointer_is_dev(*b->field)) {
            continue;
        }
        swaps[i].user = *b->field;
        swaps[i].host = alloc_host_buf(b->count, b->datatype, &swaps[i].raw);
        *b->field = swaps[i].host;
        if (is_blocking && (b->dir & COLL_BUF_READ)) {
            mpi_errno = MPIR_Localcopy(swaps[i].user, b->count, b->datatype,
                                       swaps[i].host, b->count, b->datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    mpi_errno = fn(coll_sig, cnt);
    MPIR_ERR_CHECK(mpi_errno);

    if (is_blocking) {
        for (int i = 0; i < bufs.n; i++) {
            struct coll_buf *b = &bufs.bufs[i];
            if (swaps[i].user && (b->dir & COLL_BUF_WRITE)) {
                mpi_errno = MPIR_Localcopy(swaps[i].host, b->count, b->datatype,
                                           swaps[i].user, b->count, b->datatype);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    } else {
        mpi_errno = add_swap_steps(coll_sig, &bufs, swaps);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    for (int i = 0; i < bufs.n; i++) {
        if (swaps[i].user) {
            *bufs.bufs[i].field = swaps[i].user;
            /* NULL once the schedule owns it */
            MPL_free(swaps[i].raw);
        }
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Adds the copy-in steps at the front of the schedule auto built, and the copy-out steps at
 * the back after a barrier (normal) or fence (gentran), and hands the host buffers to the
 * schedule. */
static int add_swap_steps(MPIR_Csel_coll_sig_s * coll_sig, struct coll_bufs *bufs,
                          struct swap_buf *swaps)
{
    int mpi_errno = MPI_SUCCESS;

    if (coll_sig->sched_type == MPIR_SCHED_NORMAL) {
        MPIR_Sched_t s = coll_sig->sched;
        for (int i = 0; i < bufs->n; i++) {
            if (swaps[i].user) {
                MPIR_Sched_add_buffer(s, swaps[i].raw);
                swaps[i].raw = NULL;
            }
        }

        int n = 0;
        for (int i = 0; i < bufs->n; i++) {
            struct coll_buf *b = &bufs->bufs[i];
            if (swaps[i].user && (b->dir & COLL_BUF_READ)) {
                mpi_errno = MPIR_Sched_copy(swaps[i].user, b->count, b->datatype,
                                            swaps[i].host, b->count, b->datatype, s);
                MPIR_ERR_CHECK(mpi_errno);
                n++;
            }
        }
        mpi_errno = MPIR_Sched_move_to_front(s, n);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Sched_barrier(s);
        MPIR_ERR_CHECK(mpi_errno);
        for (int i = 0; i < bufs->n; i++) {
            struct coll_buf *b = &bufs->bufs[i];
            if (swaps[i].user && (b->dir & COLL_BUF_WRITE)) {
                mpi_errno = MPIR_Sched_copy(swaps[i].host, b->count, b->datatype,
                                            swaps[i].user, b->count, b->datatype, s);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    } else if (coll_sig->sched_type == MPIR_SCHED_GENTRAN) {
        MPIR_TSP_sched_t s = coll_sig->sched;
        for (int i = 0; i < bufs->n; i++) {
            if (swaps[i].user) {
                MPIR_TSP_sched_add_buffer(s, swaps[i].raw);
                swaps[i].raw = NULL;
            }
        }

        for (int i = 0; i < bufs->n; i++) {
            struct coll_buf *b = &bufs->bufs[i];
            if (swaps[i].user && (b->dir & COLL_BUF_READ)) {
                mpi_errno = MPIR_TSP_sched_localcopy_front(swaps[i].user, b->count, b->datatype,
                                                           swaps[i].host, b->count, b->datatype, s);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        mpi_errno = MPIR_TSP_sched_fence(s);
        MPIR_ERR_CHECK(mpi_errno);
        for (int i = 0; i < bufs->n; i++) {
            struct coll_buf *b = &bufs->bufs[i];
            if (swaps[i].user && (b->dir & COLL_BUF_WRITE)) {
                int vtx_id;
                mpi_errno = MPIR_TSP_sched_localcopy(swaps[i].host, b->count, b->datatype,
                                                     swaps[i].user, b->count, b->datatype,
                                                     s, 0, NULL, &vtx_id);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    } else {
        /* MPIR_SCHED_REQUEST has no steps to add; nothing produces it for now */
        MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_alloc_buffer, plus the pointer as allocated, which it shifts by the datatype's lb */
static void *alloc_host_buf(MPI_Aint count, MPI_Datatype datatype, void **raw)
{
    MPI_Aint extent, true_lb, true_extent;
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPI_Aint shift = true_lb;
    if (count > 1 && extent < 0) {
        shift -= (-extent) * (count - 1);
    }

    void *host_buf = MPIR_alloc_buffer(count, datatype);
    *raw = (char *) host_buf + shift;
    return host_buf;
}

/* Fills bufs with the buffers this rank uses. Returns false for the collectives it doesn't
 * describe yet: intercomm, alltoallw, neighborhood, and v-collectives with negative
 * displacements. Those never set the gpu flags. */
static bool get_coll_bufs(MPIR_Csel_coll_sig_s * coll_sig, struct coll_bufs *bufs)
{
    MPIR_Comm *comm = coll_sig->comm_ptr;
    int rank = comm->rank;
    int size = comm->local_size;
    MPI_Aint span;

    bufs->n = 0;
    if (comm->comm_kind != MPIR_COMM_KIND__INTRACOMM) {
        return false;
    }
#define U(name) (&coll_sig->u.name)
    switch (coll_sig->coll_type) {
          COLL_TYPE_INTRA_CASE(BCAST):
            add_buf(bufs, (const void **) &U(bcast)->buffer, U(bcast)->count,
                    U(bcast)->datatype, rank == U(bcast)->root ? COLL_BUF_READ : COLL_BUF_WRITE);
            break;
          COLL_TYPE_INTRA_CASE(REDUCE):
            add_send_recv(bufs, &U(reduce)->sendbuf, U(reduce)->count, U(reduce)->datatype,
                          &U(reduce)->recvbuf, U(reduce)->count, U(reduce)->datatype,
                          rank == U(reduce)->root);
            break;
          COLL_TYPE_INTRA_CASE(ALLREDUCE):
            add_send_recv(bufs, &U(allreduce)->sendbuf, U(allreduce)->count,
                          U(allreduce)->datatype, &U(allreduce)->recvbuf, U(allreduce)->count,
                          U(allreduce)->datatype, true);
            break;
          COLL_TYPE_INTRA_CASE(SCAN):
            add_send_recv(bufs, &U(scan)->sendbuf, U(scan)->count, U(scan)->datatype,
                          &U(scan)->recvbuf, U(scan)->count, U(scan)->datatype, true);
            break;
          COLL_TYPE_INTRA_CASE(EXSCAN):
            if (rank > 0) {
                add_send_recv(bufs, &U(exscan)->sendbuf, U(exscan)->count, U(exscan)->datatype,
                              &U(exscan)->recvbuf, U(exscan)->count, U(exscan)->datatype, true);
            } else if (U(exscan)->sendbuf == MPI_IN_PLACE) {
                /* rank 0 gets no result: its recvbuf is only input, and must stay unchanged */
                add_buf(bufs, (const void **) &U(exscan)->recvbuf, U(exscan)->count,
                        U(exscan)->datatype, COLL_BUF_READ);
            } else {
                add_buf(bufs, &U(exscan)->sendbuf, U(exscan)->count, U(exscan)->datatype,
                        COLL_BUF_READ);
            }
            break;
          COLL_TYPE_INTRA_CASE(REDUCE_SCATTER):
            {
                MPI_Aint total = 0;
                for (int i = 0; i < size; i++) {
                    total += U(reduce_scatter)->recvcounts[i];
                }
                if (U(reduce_scatter)->sendbuf == MPI_IN_PLACE) {
                    add_buf(bufs, (const void **) &U(reduce_scatter)->recvbuf, total,
                            U(reduce_scatter)->datatype, COLL_BUF_RW);
                } else {
                    add_buf(bufs, &U(reduce_scatter)->sendbuf, total,
                            U(reduce_scatter)->datatype, COLL_BUF_READ);
                    add_buf(bufs, (const void **) &U(reduce_scatter)->recvbuf,
                            U(reduce_scatter)->recvcounts[rank], U(reduce_scatter)->datatype,
                            COLL_BUF_WRITE);
                }
            }
            break;
          COLL_TYPE_INTRA_CASE(REDUCE_SCATTER_BLOCK):
            {
                MPI_Aint count = U(reduce_scatter_block)->recvcount;
                MPI_Datatype datatype = U(reduce_scatter_block)->datatype;
                if (U(reduce_scatter_block)->sendbuf == MPI_IN_PLACE) {
                    add_buf(bufs, (const void **) &U(reduce_scatter_block)->recvbuf,
                            count * size, datatype, COLL_BUF_RW);
                } else {
                    add_buf(bufs, &U(reduce_scatter_block)->sendbuf, count * size, datatype,
                            COLL_BUF_READ);
                    add_buf(bufs, (const void **) &U(reduce_scatter_block)->recvbuf, count,
                            datatype, COLL_BUF_WRITE);
                }
            }
            break;
          COLL_TYPE_INTRA_CASE(ALLGATHER):
            add_send_recv(bufs, &U(allgather)->sendbuf, U(allgather)->sendcount,
                          U(allgather)->sendtype, &U(allgather)->recvbuf,
                          U(allgather)->recvcount * size, U(allgather)->recvtype, true);
            break;
          COLL_TYPE_INTRA_CASE(ALLGATHERV):
            span = get_span(U(allgatherv)->recvcounts, U(allgatherv)->displs, size);
            if (span < 0) {
                return false;
            }
            if (U(allgatherv)->sendbuf != MPI_IN_PLACE) {
                add_buf(bufs, &U(allgatherv)->sendbuf, U(allgatherv)->sendcount,
                        U(allgatherv)->sendtype, COLL_BUF_READ);
            }
            add_buf(bufs, (const void **) &U(allgatherv)->recvbuf, span,
                    U(allgatherv)->recvtype, COLL_BUF_RW);
            break;
          COLL_TYPE_INTRA_CASE(GATHER):
            add_send_recv(bufs, &U(gather)->sendbuf, U(gather)->sendcount, U(gather)->sendtype,
                          &U(gather)->recvbuf, U(gather)->recvcount * size, U(gather)->recvtype,
                          rank == U(gather)->root);
            break;
          COLL_TYPE_INTRA_CASE(GATHERV):
            if (U(gatherv)->sendbuf != MPI_IN_PLACE) {
                add_buf(bufs, &U(gatherv)->sendbuf, U(gatherv)->sendcount, U(gatherv)->sendtype,
                        COLL_BUF_READ);
            }
            if (rank == U(gatherv)->root) {
                span = get_span(U(gatherv)->recvcounts, U(gatherv)->displs, size);
                if (span < 0) {
                    return false;
                }
                add_buf(bufs, (const void **) &U(gatherv)->recvbuf, span, U(gatherv)->recvtype,
                        COLL_BUF_RW);
            }
            break;
          COLL_TYPE_INTRA_CASE(SCATTER):
            if (rank == U(scatter)->root) {
                add_buf(bufs, &U(scatter)->sendbuf, U(scatter)->sendcount * size,
                        U(scatter)->sendtype, COLL_BUF_READ);
            }
            if (U(scatter)->recvbuf != MPI_IN_PLACE) {
                add_buf(bufs, (const void **) &U(scatter)->recvbuf, U(scatter)->recvcount,
                        U(scatter)->recvtype, COLL_BUF_WRITE);
            }
            break;
          COLL_TYPE_INTRA_CASE(SCATTERV):
            if (rank == U(scatterv)->root) {
                span = get_span(U(scatterv)->sendcounts, U(scatterv)->displs, size);
                if (span < 0) {
                    return false;
                }
                add_buf(bufs, &U(scatterv)->sendbuf, span, U(scatterv)->sendtype, COLL_BUF_READ);
            }
            if (U(scatterv)->recvbuf != MPI_IN_PLACE) {
                add_buf(bufs, (const void **) &U(scatterv)->recvbuf, U(scatterv)->recvcount,
                        U(scatterv)->recvtype, COLL_BUF_WRITE);
            }
            break;
          COLL_TYPE_INTRA_CASE(ALLTOALL):
            add_send_recv(bufs, &U(alltoall)->sendbuf, U(alltoall)->sendcount * size,
                          U(alltoall)->sendtype, &U(alltoall)->recvbuf,
                          U(alltoall)->recvcount * size, U(alltoall)->recvtype, true);
            break;
          COLL_TYPE_INTRA_CASE(ALLTOALLV):
            if (U(alltoallv)->sendbuf != MPI_IN_PLACE) {
                span = get_span(U(alltoallv)->sendcounts, U(alltoallv)->sdispls, size);
                if (span < 0) {
                    return false;
                }
                add_buf(bufs, &U(alltoallv)->sendbuf, span, U(alltoallv)->sendtype, COLL_BUF_READ);
            }
            span = get_span(U(alltoallv)->recvcounts, U(alltoallv)->rdispls, size);
            if (span < 0) {
                return false;
            }
            add_buf(bufs, (const void **) &U(alltoallv)->recvbuf, span, U(alltoallv)->recvtype,
                    COLL_BUF_RW);
            break;
        default:
            return false;
    }
#undef U

    return true;
}

/* The common pattern: a sendbuf that may be MPI_IN_PLACE, and a recvbuf if has_recv. With
 * MPI_IN_PLACE the recvbuf holds the input too. */
static void add_send_recv(struct coll_bufs *bufs, const void **sendbuf, MPI_Aint sendcount,
                          MPI_Datatype sendtype, void **recvbuf, MPI_Aint recvcount,
                          MPI_Datatype recvtype, bool has_recv)
{
    bool inplace = (*sendbuf == MPI_IN_PLACE);
    if (!inplace) {
        add_buf(bufs, sendbuf, sendcount, sendtype, COLL_BUF_READ);
    }
    if (has_recv) {
        add_buf(bufs, (const void **) recvbuf, recvcount, recvtype,
                inplace ? COLL_BUF_RW : COLL_BUF_WRITE);
    }
}

/* Skips empty buffers. The field is cast to non-const so that buffer_swap can redirect it. */
static void add_buf(struct coll_bufs *bufs, const void **field, MPI_Aint count,
                    MPI_Datatype datatype, int dir)
{
    if (count == 0 || *field == NULL) {
        return;
    }
    MPIR_Assert(bufs->n < MAX_COLL_BUFS);
    struct coll_buf *b = &bufs->bufs[bufs->n++];
    b->field = (void **) field;
    b->count = count;
    b->datatype = datatype;
    b->dir = dir;
}

/* The span of a v-collective buffer, in elements of its datatype: from the buffer start to
 * the end of the last block. Returns -1 if a block starts before the buffer start. */
static MPI_Aint get_span(const MPI_Aint * counts, const MPI_Aint * displs, int n)
{
    MPI_Aint span = 0;
    for (int i = 0; i < n; i++) {
        if (counts[i] > 0) {
            if (displs[i] < 0) {
                return -1;
            }
            span = MPL_MAX(span, displs[i] + counts[i]);
        }
    }
    return span;
}
