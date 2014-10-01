/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test is a simplification of the one in perf/manyrma.c that tests
   for correct handling of the case where many RMA operations occur between
   synchronization events.
   This is one of the ways that RMA may be used, and is used in the
   reference implementation of the graph500 benchmark.
*/
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
sync_t syncChoice = SYNC_FENCE;
rma_t rmaChoice = RMA_ACC;

static int verbose = 0;

void RunAccFence(MPI_Win win, int destRank, int cnt, int sz);
void RunAccLock(MPI_Win win, int destRank, int cnt, int sz);
void RunPutFence(MPI_Win win, int destRank, int cnt, int sz);
void RunPutLock(MPI_Win win, int destRank, int cnt, int sz);
void RunAccPSCW(MPI_Win win, int destRank, int cnt, int sz,
                MPI_Group exposureGroup, MPI_Group accessGroup);
void RunPutPSCW(MPI_Win win, int destRank, int cnt, int sz,
                MPI_Group exposureGroup, MPI_Group accessGroup);

int main(int argc, char *argv[])
{
    int arraysize, i, cnt, sz, maxCount = MAX_COUNT, *arraybuffer;
    int wrank, wsize, destRank, srcRank;
    MPI_Win win;
    MPI_Group wgroup, accessGroup, exposureGroup;
    int maxSz = MAX_RMA_SIZE;
    double start, end;

    MPI_Init(&argc, &argv);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-put") == 0) {
            if (rmaChoice == RMA_ALL)
                rmaChoice = RMA_NONE;
            rmaChoice |= RMA_PUT;
        }
        else if (strcmp(argv[i], "-acc") == 0) {
            if (rmaChoice == RMA_ALL)
                rmaChoice = RMA_NONE;
            rmaChoice |= RMA_ACC;
        }
        else if (strcmp(argv[i], "-fence") == 0) {
            if (syncChoice == SYNC_ALL)
                syncChoice = SYNC_NONE;
            syncChoice |= SYNC_FENCE;
        }
        else if (strcmp(argv[i], "-lock") == 0) {
            if (syncChoice == SYNC_ALL)
                syncChoice = SYNC_NONE;
            syncChoice |= SYNC_LOCK;
        }
        else if (strcmp(argv[i], "-pscw") == 0) {
            if (syncChoice == SYNC_ALL)
                syncChoice = SYNC_NONE;
            syncChoice |= SYNC_PSCW;
        }
        else if (strcmp(argv[i], "-maxsz") == 0) {
            i++;
            maxSz = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-maxcount") == 0) {
            i++;
            maxCount = atoi(argv[i]);
        }
        else {
            fprintf(stderr, "Unrecognized argument %s\n", argv[i]);
            fprintf(stderr,
                    "%s [ -put ] [ -acc ] [ -lock ] [ -fence ] [ -pscw ] [ -maxsz msgsize ]\n",
                    argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

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
#ifdef USE_WIN_ALLOCATE
    MPI_Win_allocate(arraysize * sizeof(int), (int) sizeof(int), MPI_INFO_NULL,
                     MPI_COMM_WORLD, &arraybuffer, &win);
    if (!arraybuffer) {
        fprintf(stderr, "Unable to allocate %d words\n", arraysize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
#else
    arraybuffer = (int *) malloc(arraysize * sizeof(int));
    if (!arraybuffer) {
        fprintf(stderr, "Unable to allocate %d words\n", arraysize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Win_create(arraybuffer, arraysize * sizeof(int), (int) sizeof(int),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);
#endif

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

#ifndef USE_WIN_ALLOCATE
    free(arraybuffer);
#endif

    MPI_Group_free(&accessGroup);
    MPI_Group_free(&exposureGroup);

    /* If we get here without timing out or failing, we succeeded */
    if (wrank == 0)
        printf(" No Errors\n");

    MPI_Finalize();
    return 0;
}


void RunAccFence(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j, one = 1;

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_fence(0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Accumulate(&one, sz, MPI_INT, destRank, j, sz, MPI_INT, MPI_SUM, win);
            j += sz;
        }
        MPI_Win_fence(0, win);
    }
}

void RunAccLock(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j, one = 1;

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_lock(MPI_LOCK_SHARED, destRank, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Accumulate(&one, sz, MPI_INT, destRank, j, sz, MPI_INT, MPI_SUM, win);
            j += sz;
        }
        MPI_Win_unlock(destRank, win);
    }
}

void RunPutFence(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j, one = 1;

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_fence(0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Put(&one, sz, MPI_INT, destRank, j, sz, MPI_INT, win);
            j += sz;
        }
        MPI_Win_fence(0, win);
    }
}

void RunPutLock(MPI_Win win, int destRank, int cnt, int sz)
{
    int k, i, j, one = 1;

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_lock(MPI_LOCK_SHARED, destRank, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Put(&one, sz, MPI_INT, destRank, j, sz, MPI_INT, win);
            j += sz;
        }
        MPI_Win_unlock(destRank, win);
    }
}

void RunPutPSCW(MPI_Win win, int destRank, int cnt, int sz,
                MPI_Group exposureGroup, MPI_Group accessGroup)
{
    int k, i, j, one = 1;

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_post(exposureGroup, 0, win);
        MPI_Win_start(accessGroup, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Put(&one, sz, MPI_INT, destRank, j, sz, MPI_INT, win);
            j += sz;
        }
        MPI_Win_complete(win);
        MPI_Win_wait(win);
    }
}

void RunAccPSCW(MPI_Win win, int destRank, int cnt, int sz,
                MPI_Group exposureGroup, MPI_Group accessGroup)
{
    int k, i, j, one = 1;

    for (k = 0; k < MAX_RUNS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_post(exposureGroup, 0, win);
        MPI_Win_start(accessGroup, 0, win);
        j = 0;
        for (i = 0; i < cnt; i++) {
            MPI_Accumulate(&one, sz, MPI_INT, destRank, j, sz, MPI_INT, MPI_SUM, win);
            j += sz;
        }
        MPI_Win_complete(win);
        MPI_Win_wait(win);
    }
}
