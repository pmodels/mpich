/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This test is a simplification of the one in perf/manyrma.c that tests
   for correct handling of the case where many RMA operations occur between
   synchronization events.
   This is one of the ways that RMA may be used, and is used in the
   reference implementation of the graph500 benchmark.
*/
#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run rma_manyrma2
int run(const char *arg);
#endif

#define MAX_COUNT 65536*4/16
#define MAX_RMA_SIZE 2  /* 16 in manyrma performance test */
#define MAX_RUNS 8
#define MAX_ITER_TIME  5.0      /* seconds */

typedef enum { SYNC_NONE = 0,
    SYNC_ALL = -1, SYNC_FENCE = 1, SYNC_LOCK = 2, SYNC_PSCW = 4
} sync_t;
typedef enum { RMA_NONE = 0, RMA_ALL = -1, RMA_PUT = 1, RMA_ACC = 2, RMA_GET = 4 } rma_t;
/* Note GET not yet implemented */
/* By default, run only a subset of the available tests, to keep the
   total runtime reasonably short.  Command line arguments may be used
   to run other tests. */
static sync_t syncChoice = SYNC_FENCE;
static rma_t rmaChoice = RMA_ACC;

static int verbose = 0;
static int use_win_allocate = 0;

static void RunAccFence(MPI_Win win, int destRank, int cnt, int sz);
static void RunAccLock(MPI_Win win, int destRank, int cnt, int sz);
static void RunPutFence(MPI_Win win, int destRank, int cnt, int sz);
static void RunPutLock(MPI_Win win, int destRank, int cnt, int sz);
static void RunAccPSCW(MPI_Win win, int destRank, int cnt, int sz,
                       MPI_Group exposureGroup, MPI_Group accessGroup);
static void RunPutPSCW(MPI_Win win, int destRank, int cnt, int sz,
                       MPI_Group exposureGroup, MPI_Group accessGroup);

int run(const char *arg)
{
    int arraysize, i, cnt, sz, maxCount = MAX_COUNT, *arraybuffer;
    int wrank, wsize, destRank, srcRank;
    MPI_Win win;
    MPI_Group wgroup, accessGroup, exposureGroup;
    int maxSz = MAX_RMA_SIZE;
    double start, end;

    if (rmaChoice == RMA_ALL)
        rmaChoice = RMA_NONE;
    if (syncChoice == SYNC_ALL)
        syncChoice = SYNC_NONE;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    use_win_allocate = MTestArgListGetInt_with_default(head, "use-win-allocate", 0);
    if (MTestArgListGetInt_with_default(head, "put", 0)) {
        rmaChoice |= RMA_PUT;
    }
    if (MTestArgListGetInt_with_default(head, "acc", 0)) {
        rmaChoice |= RMA_ACC;
    }
    if (MTestArgListGetInt_with_default(head, "fence", 0)) {
        syncChoice |= SYNC_FENCE;
    }
    if (MTestArgListGetInt_with_default(head, "lock", 0)) {
        syncChoice |= SYNC_LOCK;
    }
    if (MTestArgListGetInt_with_default(head, "pscw", 0)) {
        syncChoice |= SYNC_PSCW;
    }
    maxSz = MTestArgListGetInt_with_default(head, "maxsz", MAX_RMA_SIZE);
    maxCount = MTestArgListGetInt_with_default(head, "maxcount", MAX_COUNT);
    MTestArgListDestroy(head);

    if (rmaChoice == RMA_NONE)
        rmaChoice = RMA_ALL;
    if (syncChoice == SYNC_NONE)
        syncChoice = SYNC_ALL;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    destRank = wrank + 1;
    while (destRank >= wsize)
        destRank = destRank - wsize;
    srcRank = wrank - 1;
    if (srcRank < 0)
        srcRank += wsize;

    /* Create groups for PSCW */
    MPI_Comm_group(MPI_COMM_WORLD, &wgroup);
    MPI_Group_incl(wgroup, 1, &destRank, &accessGroup);
    MPI_Group_incl(wgroup, 1, &srcRank, &exposureGroup);
    MPI_Group_free(&wgroup);

    arraysize = maxSz * MAX_COUNT;
    if (use_win_allocate) {
        MPI_Win_allocate(arraysize * sizeof(int), (int) sizeof(int), MPI_INFO_NULL,
                         MPI_COMM_WORLD, &arraybuffer, &win);
        if (!arraybuffer) {
            fprintf(stderr, "Unable to allocate %d words\n", arraysize);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    } else {
        arraybuffer = (int *) malloc(arraysize * sizeof(int));
        if (!arraybuffer) {
            fprintf(stderr, "Unable to allocate %d words\n", arraysize);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Win_create(arraybuffer, arraysize * sizeof(int), (int) sizeof(int),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    }

    if (maxCount > MAX_COUNT) {
        fprintf(stderr, "MaxCount must not exceed %d\n", MAX_COUNT);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if ((syncChoice & SYNC_FENCE) && (rmaChoice & RMA_ACC)) {
        for (sz = 1; sz <= maxSz; sz = sz + sz) {
            if (wrank == 0 && verbose)
                printf("Accumulate with fence, %d elements\n", sz);
            for (cnt = 1; cnt <= maxCount; cnt *= 2) {
                start = MPI_Wtime();
                RunAccFence(win, destRank, cnt, sz);
                end = MPI_Wtime();
                if (end - start > MAX_ITER_TIME)
                    break;
            }
        }
    }

    if ((syncChoice & SYNC_LOCK) && (rmaChoice & RMA_ACC)) {
        for (sz = 1; sz <= maxSz; sz = sz + sz) {
            if (wrank == 0 && verbose)
                printf("Accumulate with lock, %d elements\n", sz);
            for (cnt = 1; cnt <= maxCount; cnt *= 2) {
                start = MPI_Wtime();
                RunAccLock(win, destRank, cnt, sz);
                end = MPI_Wtime();
                if (end - start > MAX_ITER_TIME)
                    break;
            }
        }
    }

    if ((syncChoice & SYNC_FENCE) && (rmaChoice & RMA_PUT)) {
        for (sz = 1; sz <= maxSz; sz = sz + sz) {
            if (wrank == 0 && verbose)
                printf("Put with fence, %d elements\n", sz);
            for (cnt = 1; cnt <= maxCount; cnt *= 2) {
                start = MPI_Wtime();
                RunPutFence(win, destRank, cnt, sz);
                end = MPI_Wtime();
                if (end - start > MAX_ITER_TIME)
                    break;
            }
        }
    }

    if ((syncChoice & SYNC_LOCK) && (rmaChoice & RMA_PUT)) {
        for (sz = 1; sz <= maxSz; sz = sz + sz) {
            if (wrank == 0 && verbose)
                printf("Put with lock, %d elements\n", sz);
            for (cnt = 1; cnt <= maxCount; cnt *= 2) {
                start = MPI_Wtime();
                RunPutLock(win, destRank, cnt, sz);
                end = MPI_Wtime();
                if (end - start > MAX_ITER_TIME)
                    break;
            }
        }
    }

    if ((syncChoice & SYNC_PSCW) && (rmaChoice & RMA_PUT)) {
        for (sz = 1; sz <= maxSz; sz = sz + sz) {
            if (wrank == 0 && verbose)
                printf("Put with pscw, %d elements\n", sz);
            for (cnt = 1; cnt <= maxCount; cnt *= 2) {
                start = MPI_Wtime();
                RunPutPSCW(win, destRank, cnt, sz, exposureGroup, accessGroup);
                end = MPI_Wtime();
                if (end - start > MAX_ITER_TIME)
                    break;
            }
        }
    }

    if ((syncChoice & SYNC_PSCW) && (rmaChoice & RMA_ACC)) {
        for (sz = 1; sz <= maxSz; sz = sz + sz) {
            if (wrank == 0 && verbose)
                printf("Accumulate with pscw, %d elements\n", sz);
            for (cnt = 1; cnt <= maxCount; cnt *= 2) {
                start = MPI_Wtime();
                RunAccPSCW(win, destRank, cnt, sz, exposureGroup, accessGroup);
                end = MPI_Wtime();
                if (end - start > MAX_ITER_TIME)
                    break;
            }
        }
    }

    MPI_Win_free(&win);

    if (!use_win_allocate) {
        free(arraybuffer);
    }

    MPI_Group_free(&accessGroup);
    MPI_Group_free(&exposureGroup);

    return 0;
}


static void RunAccFence(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j;
    int *buf = malloc(sz * sizeof(int));

    for (i = 0; i < sz; i++) {
        buf[i] = i;
    }

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_fence(0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Accumulate(buf, sz, MPI_INT, destRank, j, sz, MPI_INT, MPI_SUM, win);
            j += sz;
        }
        MPI_Win_fence(0, win);
    }

    free(buf);
}

static void RunAccLock(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j;
    int *buf = malloc(sz * sizeof(int));

    for (i = 0; i < sz; i++) {
        buf[i] = i;
    }

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_lock(MPI_LOCK_SHARED, destRank, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Accumulate(buf, sz, MPI_INT, destRank, j, sz, MPI_INT, MPI_SUM, win);
            j += sz;
        }
        MPI_Win_unlock(destRank, win);
    }

    free(buf);
}

static void RunPutFence(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j;
    int *buf = malloc(sz * sizeof(int));

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_fence(0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Put(buf, sz, MPI_INT, destRank, j, sz, MPI_INT, win);
            j += sz;
        }
        MPI_Win_fence(0, win);
    }

    free(buf);
}

static void RunPutLock(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j;
    int *buf = malloc(sz * sizeof(int));

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_lock(MPI_LOCK_SHARED, destRank, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Put(buf, sz, MPI_INT, destRank, j, sz, MPI_INT, win);
            j += sz;
        }
        MPI_Win_unlock(destRank, win);
    }

    free(buf);
}

static void RunPutPSCW(MPI_Win win, int destRank, int cnt, int sz,
                       MPI_Group exposureGroup, MPI_Group accessGroup)
{
    int k, i, j;
    int *buf = malloc(sz * sizeof(int));

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_post(exposureGroup, 0, win);
        MPI_Win_start(accessGroup, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Put(buf, sz, MPI_INT, destRank, j, sz, MPI_INT, win);
            j += sz;
        }
        MPI_Win_complete(win);
        MPI_Win_wait(win);
    }

    free(buf);
}

static void RunAccPSCW(MPI_Win win, int destRank, int cnt, int sz,
                       MPI_Group exposureGroup, MPI_Group accessGroup)
{
    int k, i, j;
    int *buf = malloc(sz * sizeof(int));

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_post(exposureGroup, 0, win);
        MPI_Win_start(accessGroup, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Accumulate(buf, sz, MPI_INT, destRank, j, sz, MPI_INT, MPI_SUM, win);
            j += sz;
        }
        MPI_Win_complete(win);
        MPI_Win_wait(win);
    }

    free(buf);
}
