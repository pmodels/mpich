/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

#define ITER 10000
#define BUF_CNT 1
int local_buf[BUF_CNT], result_addr[BUF_CNT];
#ifdef TEST_CAS
int compare_buf[BUF_CNT];
#endif

const int verbose = 0;

/* This test checks the remote completion of flush with RMA write-like operations
 * (PUT, ACC, GET_ACC, FOP, CAS), and confirms result by shm load.
 * 1. P(target) and P(checker) allocate a shared window, and
 *    then create a global window with P(origin) by using the shared window buffer.
 * 2. P(origin) issues RMA operations and flush to P(target) through the global
 *    window and then call send-recv to synchronize with P(checker).
 * 3. P(checker) then checks the result through shm window by local load. */

int rank = -1, nproc = 0;
int origin = -1, target = -1, checker = -1;
MPI_Win win = MPI_WIN_NULL, shm_win = MPI_WIN_NULL;
int *shm_target_base = NULL, *my_base = NULL;

/* Define operation name for error message */
#ifdef TEST_PUT
const char *rma_name = "Put";
#elif defined(TEST_ACC)
const char *rma_name = "Accumulate";
#elif defined(TEST_GACC)
const char *rma_name = "Get_accumulate";
#elif defined(TEST_FOP)
const char *rma_name = "Fetch_and_op";
#elif defined(TEST_CAS)
const char *rma_name = "Compare_and_swap";
#else
const char *rma_name = "None";
#endif

/* Issue functions for different RMA operations */
#ifdef TEST_PUT
static inline void issue_rma_op(int i)
{
    MPI_Put(&local_buf[i], 1, MPI_INT, target, i, 1, MPI_INT, win);
}
#elif defined(TEST_ACC)
static inline void issue_rma_op(int i)
{
    MPI_Accumulate(&local_buf[i], 1, MPI_INT, target, i, 1, MPI_INT, MPI_REPLACE, win);
}
#elif defined(TEST_GACC)
static inline void issue_rma_op(int i)
{
    MPI_Get_accumulate(&local_buf[i], 1, MPI_INT, &result_addr[i], 1, MPI_INT, target, i,
                       1, MPI_INT, MPI_REPLACE, win);
}
#elif defined(TEST_FOP)
static inline void issue_rma_op(int i)
{
    MPI_Fetch_and_op(&local_buf[i], &result_addr[i], MPI_INT, target, i, MPI_REPLACE, win);
}
#elif defined(TEST_CAS)
static inline void issue_rma_op(int i)
{
    compare_buf[i] = i;        /* always equal to window value, thus swap happens */
    MPI_Compare_and_swap(&local_buf[i], &compare_buf[i], &result_addr[i], MPI_INT, target, i, win);
}
#endif


/* Local check function for GET-like operations */
#if defined(TEST_GACC) || defined(TEST_FOP) || defined(TEST_CAS)

/* Check local result buffer for GET-like operations */
static int check_local_result(int iter)
{
    int i = 0;
    int errors = 0;

    for (i = 0; i < BUF_CNT; i++) {
        if (result_addr[i] != i) {
            printf("rank %d (iter %d) - check %s, got result_addr[%d] = %d, expected %d\n",
                   rank, iter, rma_name, i, result_addr[i], i);
            errors++;
        }
    }
    return errors;
}

#else
#define check_local_result(iter) (0)
#endif

static int run_test()
{
    int i = 0, x = 0;
    int errors = 0;
    int sbuf = 0, rbuf = 0;
    MPI_Status stat;

    for (x = 0; x < ITER; x++) {
        /* 1. Target resets window data */
        if (rank == target) {
            for (i = 0; i < BUF_CNT; i++)
                my_base[i] = i;
            MPI_Win_sync(shm_win);      /* write is done on shm window */
        }

        MPI_Barrier(MPI_COMM_WORLD);

        /* 2. Every one resets local data */
        for (i = 0; i < BUF_CNT; i++) {
            local_buf[i] = BUF_CNT + x * BUF_CNT + i;
            result_addr[i] = 0;
        }

        /* 3. Origin issues RMA operation to target */
        if (rank == origin) {
            /* 3-1. Issue RMA. */
            for (i = 0; i < BUF_CNT; i++) {
                issue_rma_op(i);
            }
            MPI_Win_flush(target, win);

            /* 3-2. Check local result buffer. */
            errors += check_local_result(x);

            /* sync with checker */
            MPI_Send(&sbuf, 1, MPI_INT, checker, 999, MPI_COMM_WORLD);
        }

        /* 4. Checker confirms result on target */
        if (rank == checker) {
            /* sync with origin */
            MPI_Recv(&rbuf, 1, MPI_INT, origin, 999, MPI_COMM_WORLD, &stat);

            MPI_Win_sync(shm_win);

            for (i = 0; i < BUF_CNT; i++) {
                if (shm_target_base[i] != local_buf[i]) {
                    printf("rank %d (iter %d) - check %s, got shm_target_base[%d] = %d, "
                           "expected %d\n", rank, x, rma_name, i, shm_target_base[i], local_buf[i]);
                    errors++;
                }
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    return errors;
}

int main(int argc, char *argv[])
{
    int i;
    int errors = 0, all_errors = 0;
    MPI_Comm shm_comm = MPI_COMM_NULL;
    int shm_rank;
    int *shm_ranks = NULL, *shm_root_ranks = NULL;
    int win_size = sizeof(int) * BUF_CNT;
    int win_unit = sizeof(int);
    int shm_root_rank = -1, shm_target = -1, target_shm_root = -1;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc != 3) {
        if (rank == 0)
            printf("Error: must be run with three processes\n");
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

#if !defined(TEST_PUT) && !defined(TEST_ACC) && !defined(TEST_GACC) && !defined(TEST_FOP) && !defined(TEST_CAS)
    if (rank == 0)
        printf("Error: must specify operation type at compile time\n");
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Abort(MPI_COMM_WORLD, 1);
#endif

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);
    MPI_Comm_rank(shm_comm, &shm_rank);

    shm_ranks = (int *) calloc(nproc, sizeof(int));
    shm_root_ranks = (int *) calloc(nproc, sizeof(int));

    /* Identify node id */
    if (shm_rank == 0)
        shm_root_rank = rank;
    MPI_Bcast(&shm_root_rank, 1, MPI_INT, 0, shm_comm);

    /* Exchange local root rank and local rank */
    shm_ranks[rank] = shm_rank;
    shm_root_ranks[rank] = shm_root_rank;

    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, shm_ranks, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, shm_root_ranks, 1, MPI_INT, MPI_COMM_WORLD);

    /* Check if there are at least two processes in shared memory. */
    for (i = 0; i < nproc; i++) {
        if (shm_ranks[i] != 0) {
            target_shm_root = shm_root_ranks[i];
            break;
        }
    }

    /* Every process is in separate memory, we cannot create shared window. Just return. */
    if (target_shm_root < 0)
        goto exit;

    /* Identify origin, target and checker ranks.
     * the first process in shared memory is target, and the second one is checker;
     * the last process is origin.*/
    shm_target = 0;
    for (i = 0; i < nproc; i++) {
        if (shm_root_ranks[i] == target_shm_root) {
            if (shm_ranks[i] == 0) {
                target = i;
            }
            else if (shm_ranks[i] == 1) {
                checker = i;
            }
            else {
                /* all three processes are in shared memory, origin is the third one. */
                origin = i;
            }
        }
        else {
            /* origin is in separate memory. */
            origin = i;
        }
    }

    if (verbose) {
        printf("----   rank %d: origin = %d, checker = %d, target = %d, test %s\n",
               rank, origin, checker, target, rma_name);
    }

    /* Allocate shared memory among local processes, then create a global window
     * with the shared window buffers. */
    MPI_Win_allocate_shared(win_size, win_unit, MPI_INFO_NULL, shm_comm, &my_base, &shm_win);
    MPI_Win_create(my_base, win_size, win_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    /* Get address of target window on checker process. */
    if (rank == checker) {
        MPI_Aint size;
        int disp_unit;
        MPI_Win_shared_query(shm_win, shm_target, &size, &disp_unit, &shm_target_base);
        if (verbose) {
            printf("----   I am checker = %d, shm_target_base=%p\n", checker, shm_target_base);
        }
    }

    /* Start checking. */
    MPI_Win_lock_all(0, win);
    MPI_Win_lock_all(0, shm_win);

    errors = run_test();

    MPI_Win_unlock_all(shm_win);
    MPI_Win_unlock_all(win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  exit:

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    if (shm_ranks)
        free(shm_ranks);
    if (shm_root_ranks)
        free(shm_root_ranks);

    if (shm_win != MPI_WIN_NULL)
        MPI_Win_free(&shm_win);

    if (win != MPI_WIN_NULL)
        MPI_Win_free(&win);

    if (shm_comm != MPI_COMM_NULL)
        MPI_Comm_free(&shm_comm);

    MPI_Finalize();

    return 0;
}
