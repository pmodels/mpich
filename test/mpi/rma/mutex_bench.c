/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run rma_mutex_bench
int run(const char *arg);
#endif

/* ---- mcs_mutex.h ---- */

#define MCS_MUTEX_TAG 100

#ifdef ENABLE_DEBUG
#define debug_print(...) do { printf(__VA_ARGS__); } while (0)
#else
#define debug_print(...)
#endif

struct mcs_mutex_s {
    int tail_rank;
    MPI_Comm comm;
    MPI_Win window;
    int *base;
    MPI_Info win_info;
};

typedef struct mcs_mutex_s *MCS_Mutex;

#define MCS_MTX_ELEM_DISP 0
#define MCS_MTX_TAIL_DISP 1

static int MCS_Mutex_create(int tail_rank, MPI_Comm comm, MCS_Mutex * hdl_out);
static int MCS_Mutex_free(MCS_Mutex * hdl_ptr);
static int MCS_Mutex_lock(MCS_Mutex hdl);
static int MCS_Mutex_trylock(MCS_Mutex hdl, int *success);
static int MCS_Mutex_unlock(MCS_Mutex hdl);

/* ---- end mcs_mutex.h ---- */

#define NUM_ITER    1000
#define NUM_MUTEXES 1

static int use_win_shared = 0;
static int use_alloc_shm = 0;
static int use_contig_rank = 0;
static const int verbose = 0;
static double delay_ctr = 0.0;

int run(const char *arg)
{
    int rank, nproc, i;
    double t_mcs_mtx;
    MPI_Comm mtx_comm;
    MCS_Mutex mcs_mtx;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    use_win_shared = MTestArgListGetInt_with_default(head, "use-win-shared", 0);
    use_alloc_shm = MTestArgListGetInt_with_default(head, "use-alloc-shm", 0);
    use_contig_rank = MTestArgListGetInt_with_default(head, "use-contig-rank", 0);
    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (use_win_shared) {
        MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &mtx_comm);
    } else {
        mtx_comm = MPI_COMM_WORLD;
    }

    MCS_Mutex_create(0, mtx_comm, &mcs_mtx);

    MPI_Barrier(MPI_COMM_WORLD);
    t_mcs_mtx = MPI_Wtime();

    for (i = 0; i < NUM_ITER; i++) {
        /* Combining trylock and lock here is helpful for testing because it makes
         * CAS and Fetch-and-op contend for the tail pointer. */
        bool do_trylock;
        if (use_contig_rank) {
            do_trylock = (rank < nproc / 2);
        } else {
            do_trylock = (rank % 2);
        }
        if (do_trylock) {
            int success = 0;
            while (!success) {
                MCS_Mutex_trylock(mcs_mtx, &success);
            }
        } else {
            MCS_Mutex_lock(mcs_mtx);
        }
        MCS_Mutex_unlock(mcs_mtx);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    t_mcs_mtx = MPI_Wtime() - t_mcs_mtx;

    MCS_Mutex_free(&mcs_mtx);

    if (rank == 0) {
        if (verbose) {
            printf("Nproc %d, MCS Mtx = %f us\n", nproc, t_mcs_mtx / NUM_ITER * 1.0e6);
        }
    }

    if (mtx_comm != MPI_COMM_WORLD)
        MPI_Comm_free(&mtx_comm);

    return 0;
}

/* ---- implementation of mcs_mutex ---- */

/* TODO: Make these mutex operations no-ops for sequential runs */

/** Create an MCS mutex.  Collective on comm.
  *
  * @param[out] comm communicator containing all processes that will use the
  *                  mutex
  * @param[out] tail_rank rank of the process in comm that holds the tail
  *                  pointer
  * @param[out] hdl  handle to the mutex
  * @return          MPI status
  */
static int MCS_Mutex_create(int tail_rank, MPI_Comm comm, MCS_Mutex * hdl_out)
{
    int rank, nproc;
    MCS_Mutex hdl;

    hdl = malloc(sizeof(struct mcs_mutex_s));
    assert(hdl != NULL);
    hdl->win_info = MPI_INFO_NULL;

    MPI_Comm_dup(comm, &hdl->comm);

    MPI_Comm_rank(hdl->comm, &rank);
    MPI_Comm_size(hdl->comm, &nproc);

    hdl->tail_rank = tail_rank;

    if (use_win_shared) {
        MPI_Win_allocate_shared(2 * sizeof(int), sizeof(int), MPI_INFO_NULL,
                                hdl->comm, &hdl->base, &hdl->window);
    } else {
        if (use_alloc_shm) {
            MPI_Info_create(&hdl->win_info);
            MPI_Info_set(hdl->win_info, "alloc_shm", "true");
        } else {
            MPI_Info_create(&hdl->win_info);
            MPI_Info_set(hdl->win_info, "alloc_shm", "false");
        }
        MPI_Win_allocate(2 * sizeof(int), sizeof(int), hdl->win_info, hdl->comm,
                         &hdl->base, &hdl->window);
    }

    MPI_Win_lock_all(0, hdl->window);

    hdl->base[0] = MPI_PROC_NULL;
    hdl->base[1] = MPI_PROC_NULL;

    MPI_Win_sync(hdl->window);
    MPI_Barrier(hdl->comm);

    *hdl_out = hdl;
    return MPI_SUCCESS;
}


/** Free an MCS mutex.  Collective on ranks in the communicator used at the
  * time of creation.
  *
  * @param[in] hdl handle to the group that will be freed
  * @return        MPI status
  */
static int MCS_Mutex_free(MCS_Mutex * hdl_ptr)
{
    MCS_Mutex hdl = *hdl_ptr;

    MPI_Win_unlock_all(hdl->window);

    MPI_Win_free(&hdl->window);
    MPI_Comm_free(&hdl->comm);
    if (hdl->win_info != MPI_INFO_NULL) {
        MPI_Info_free(&hdl->win_info);
    }

    free(hdl);
    hdl_ptr = NULL;

    return MPI_SUCCESS;
}


/** Lock a mutex.
  *
  * @param[in] hdl   Handle to the mutex
  * @return          MPI status
  */
static int MCS_Mutex_lock(MCS_Mutex hdl)
{
    int rank, nproc;
    int prev;

    MPI_Comm_rank(hdl->comm, &rank);
    MPI_Comm_size(hdl->comm, &nproc);

    /* This store is safe, since it cannot happen concurrently with a remote
     * write */
    hdl->base[MCS_MTX_ELEM_DISP] = MPI_PROC_NULL;
    MPI_Win_sync(hdl->window);

    MPI_Fetch_and_op(&rank, &prev, MPI_INT, hdl->tail_rank, MCS_MTX_TAIL_DISP,
                     MPI_REPLACE, hdl->window);
    MPI_Win_flush(hdl->tail_rank, hdl->window);

    /* If there was a previous tail, update their next pointer and wait for
     * notification.  Otherwise, the mutex was successfully acquired. */
    if (prev != MPI_PROC_NULL) {
        /* Wait for notification */
        MPI_Status status;

        MPI_Accumulate(&rank, 1, MPI_INT, prev, MCS_MTX_ELEM_DISP, 1, MPI_INT, MPI_REPLACE,
                       hdl->window);
        MPI_Win_flush(prev, hdl->window);

        debug_print("%2d: LOCK   - waiting for notification from %d\n", rank, prev);
        MPI_Recv(NULL, 0, MPI_BYTE, prev, MCS_MUTEX_TAG, hdl->comm, &status);
    }

    debug_print("%2d: LOCK   - lock acquired\n", rank);

    return MPI_SUCCESS;
}


/** Attempt to acquire a mutex.
  *
  * @param[in] hdl   Handle to the mutex
  * @param[out] success Indicates whether the mutex was acquired
  * @return          MPI status
  */
static int MCS_Mutex_trylock(MCS_Mutex hdl, int *success)
{
    int rank, nproc;
    int tail, nil = MPI_PROC_NULL;

    MPI_Comm_rank(hdl->comm, &rank);
    MPI_Comm_size(hdl->comm, &nproc);

    /* This store is safe, since it cannot happen concurrently with a remote
     * write */
    hdl->base[MCS_MTX_ELEM_DISP] = MPI_PROC_NULL;
    MPI_Win_sync(hdl->window);

    /* Check if the lock is available and claim it if it is. */
    MPI_Compare_and_swap(&rank, &nil, &tail, MPI_INT, hdl->tail_rank,
                         MCS_MTX_TAIL_DISP, hdl->window);
    MPI_Win_flush(hdl->tail_rank, hdl->window);

    /* If the old tail was MPI_PROC_NULL, we have claimed the mutex */
    *success = (tail == nil);

    debug_print("%2d: TRYLOCK - %s\n", rank, (*success) ? "Success" : "Non-success");

    return MPI_SUCCESS;
}


/** Unlock a mutex.
  *
  * @param[in] hdl   Handle to the mutex
  * @return          MPI status
  */
static int MCS_Mutex_unlock(MCS_Mutex hdl)
{
    int rank, nproc, next;

    MPI_Comm_rank(hdl->comm, &rank);
    MPI_Comm_size(hdl->comm, &nproc);

    MPI_Win_sync(hdl->window);

    /* Read my next pointer.  FOP is used since another process may write to
     * this location concurrent with this read. */
    MPI_Fetch_and_op(NULL, &next, MPI_INT, rank, MCS_MTX_ELEM_DISP, MPI_NO_OP, hdl->window);
    MPI_Win_flush(rank, hdl->window);

    if (next == MPI_PROC_NULL) {
        int tail;
        int nil = MPI_PROC_NULL;

        /* Check if we are the at the tail of the lock queue.  If so, we're
         * done.  If not, we need to send notification. */
        MPI_Compare_and_swap(&nil, &rank, &tail, MPI_INT, hdl->tail_rank,
                             MCS_MTX_TAIL_DISP, hdl->window);
        MPI_Win_flush(hdl->tail_rank, hdl->window);

        if (tail != rank) {
            debug_print("%2d: UNLOCK - waiting for next pointer (tail = %d)\n", rank, tail);
            assert(tail >= 0 && tail < nproc);

            for (;;) {
                int flag;

                MPI_Fetch_and_op(NULL, &next, MPI_INT, rank, MCS_MTX_ELEM_DISP,
                                 MPI_NO_OP, hdl->window);

                MPI_Win_flush(rank, hdl->window);
                if (next != MPI_PROC_NULL)
                    break;

                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            }
        }
    }

    /* Notify the next waiting process */
    if (next != MPI_PROC_NULL) {
        debug_print("%2d: UNLOCK - notifying %d\n", rank, next);
        MPI_Send(NULL, 0, MPI_BYTE, next, MCS_MUTEX_TAG, hdl->comm);
    }

    debug_print("%2d: UNLOCK - lock released\n", rank);

    return MPI_SUCCESS;
}
