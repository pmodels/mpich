/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include "mpitest.h"
#if defined(HAVE_STDIO_H) || defined(STDC_HEADERS)
#include <stdio.h>
#endif
#if defined(HAVE_STDLIB_H) || defined(STDC_HEADERS)
#include <stdlib.h>
#endif
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
/* The following two includes permit the collection of resource usage
   data in the tests
 */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <errno.h>


/*
 * Utility routines for writing MPI tests.
 *
 * We check the return codes on all MPI routines (other than INIT)
 * to allow the program that uses these routines to select MPI_ERRORS_RETURN
 * as the error handler.  We do *not* set MPI_ERRORS_RETURN because
 * the code that makes use of these routines may not check return
 * codes.
 *
 */

static void MTestRMACleanup(void);
static void MTestResourceSummary(FILE *);

/* Here is where we could put the includes and definitions to enable
   memory testing */

static int dbgflag = 0;         /* Flag used for debugging */
static int wrank = -1;          /* World rank */
static int verbose = 0;         /* Message level (0 is none) */
static int returnWithVal = 0;   /* Allow programs to return with a non-zero
                                 * if there was an error (may cause problems
                                 * with some runtime systems) */
static int usageOutput = 0;     /* */

/* Provide backward portability to MPI 1 */
#ifndef MPI_VERSION
#define MPI_VERSION 1
#endif
#if MPI_VERSION < 2
#define MPI_THREAD_SINGLE 0
#endif

/*
 * Initialize and Finalize MTest
 */

/*
   Initialize MTest, initializing MPI if necessary.

 Environment Variables:
+ MPITEST_DEBUG - If set (to any value), turns on debugging output
. MPITEST_THREADLEVEL_DEFAULT - If set, use as the default "provided"
                                level of thread support.  Applies to
                                MTest_Init but not MTest_Init_thread.
- MPITEST_VERBOSE - If set to a numeric value, turns on that level of
  verbose output.  This is used by the routine 'MTestPrintfMsg'

*/
void MTest_Init_thread(int *argc, char ***argv, int required, int *provided)
{
    int flag;
    char *envval = 0;

    MPI_Initialized(&flag);
    if (!flag) {
        /* Permit an MPI that claims only MPI 1 but includes the
         * MPI_Init_thread routine (e.g., IBM MPI) */
#if MPI_VERSION >= 2 || defined(HAVE_MPI_INIT_THREAD)
        MPI_Init_thread(argc, argv, required, provided);
#else
        MPI_Init(argc, argv);
        *provided = -1;
#endif
    }
    /* Check for debugging control */
    if (getenv("MPITEST_DEBUG")) {
        dbgflag = 1;
        MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    }

    /* Check for verbose control */
    envval = getenv("MPITEST_VERBOSE");
    if (envval) {
        char *s;
        long val = strtol(envval, &s, 0);
        if (s == envval) {
            /* This is the error case for strtol */
            fprintf(stderr, "Warning: %s not valid for MPITEST_VERBOSE\n", envval);
            fflush(stderr);
        }
        else {
            if (val >= 0) {
                verbose = val;
            }
            else {
                fprintf(stderr, "Warning: %s not valid for MPITEST_VERBOSE\n", envval);
                fflush(stderr);
            }
        }
    }
    /* Check for option to return success/failure in the return value of main */
    envval = getenv("MPITEST_RETURN_WITH_CODE");
    if (envval) {
        if (strcmp(envval, "yes") == 0 ||
            strcmp(envval, "YES") == 0 ||
            strcmp(envval, "true") == 0 || strcmp(envval, "TRUE") == 0) {
            returnWithVal = 1;
        }
        else if (strcmp(envval, "no") == 0 ||
                 strcmp(envval, "NO") == 0 ||
                 strcmp(envval, "false") == 0 || strcmp(envval, "FALSE") == 0) {
            returnWithVal = 0;
        }
        else {
            fprintf(stderr, "Warning: %s not valid for MPITEST_RETURN_WITH_CODE\n", envval);
            fflush(stderr);
        }
    }

    /* Print rusage data if set */
    if (getenv("MPITEST_RUSAGE")) {
        usageOutput = 1;
    }
}

/*
 * Initialize the tests, using an MPI-1 style init.  Supports
 * MTEST_THREADLEVEL_DEFAULT to test with user-specified thread level
 */
void MTest_Init(int *argc, char ***argv)
{
    int provided;
#if MPI_VERSION >= 2 || defined(HAVE_MPI_INIT_THREAD)
    const char *str = 0;
    int threadLevel;

    threadLevel = MPI_THREAD_SINGLE;
    str = getenv("MTEST_THREADLEVEL_DEFAULT");
    if (!str)
        str = getenv("MPITEST_THREADLEVEL_DEFAULT");
    if (str && *str) {
        if (strcmp(str, "MULTIPLE") == 0 || strcmp(str, "multiple") == 0) {
            threadLevel = MPI_THREAD_MULTIPLE;
        }
        else if (strcmp(str, "SERIALIZED") == 0 || strcmp(str, "serialized") == 0) {
            threadLevel = MPI_THREAD_SERIALIZED;
        }
        else if (strcmp(str, "FUNNELED") == 0 || strcmp(str, "funneled") == 0) {
            threadLevel = MPI_THREAD_FUNNELED;
        }
        else if (strcmp(str, "SINGLE") == 0 || strcmp(str, "single") == 0) {
            threadLevel = MPI_THREAD_SINGLE;
        }
        else {
            fprintf(stderr, "Unrecognized thread level %s\n", str);
            /* Use exit since MPI_Init/Init_thread has not been called. */
            exit(1);
        }
    }
    MTest_Init_thread(argc, argv, threadLevel, &provided);
#else
    /* If the MPI_VERSION is 1, there is no MPI_THREAD_xxx defined */
    MTest_Init_thread(argc, argv, 0, &provided);
#endif
}

/*
  Finalize MTest.  errs is the number of errors on the calling process;
  this routine will write the total number of errors over all of MPI_COMM_WORLD
  to the process with rank zero, or " No Errors".
  It does *not* finalize MPI.
 */
void MTest_Finalize(int errs)
{
    int rank, toterrs, merr;

    merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (merr)
        MTestPrintError(merr);

    merr = MPI_Reduce(&errs, &toterrs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (merr)
        MTestPrintError(merr);
    if (rank == 0) {
        if (toterrs) {
            printf(" Found %d errors\n", toterrs);
        }
        else {
            printf(" No Errors\n");
        }
        fflush(stdout);
    }

    if (usageOutput)
        MTestResourceSummary(stdout);


    /* Clean up any persistent objects that we allocated */
    MTestRMACleanup();
}

/* ------------------------------------------------------------------------ */
/* This routine may be used instead of "return 0;" at the end of main;
   it allows the program to use the return value to signal success or failure.
 */
int MTestReturnValue(int errors)
{
    if (returnWithVal)
        return errors ? 1 : 0;
    return 0;
}

/* ------------------------------------------------------------------------ */

/*
 * Miscellaneous utilities, particularly to eliminate OS dependencies
 * from the tests.
 * MTestSleep(seconds)
 */
#ifdef HAVE_WINDOWS_H
#include <windows.h>
void MTestSleep(int sec)
{
    Sleep(1000 * sec);
}
#else
#include <unistd.h>
void MTestSleep(int sec)
{
    sleep(sec);
}
#endif

/* Other mtest subfiles read debug setting using this function. */
void MTestGetDbgInfo(int *_dbgflag, int *_verbose)
{
    *_dbgflag = dbgflag;
    *_verbose = verbose;
}

/* ----------------------------------------------------------------------- */

/*
 * Create communicators.  Use separate routines for inter and intra
 * communicators (there is a routine to give both)
 * Note that the routines may return MPI_COMM_NULL, so code should test for
 * that return value as well.
 *
 */
static int interCommIdx = 0;
static int intraCommIdx = 0;
static const char *intraCommName = 0;
static const char *interCommName = 0;

/*
 * Get an intracommunicator with at least min_size members.  If "allowSmaller"
 * is true, allow the communicator to be smaller than MPI_COMM_WORLD and
 * for this routine to return MPI_COMM_NULL for some values.  Returns 0 if
 * no more communicators are available.
 */
int MTestGetIntracommGeneral(MPI_Comm * comm, int min_size, int allowSmaller)
{
    int size, rank, merr;
    int done = 0;
    int isBasic = 0;

    /* The while loop allows us to skip communicators that are too small.
     * MPI_COMM_NULL is always considered large enough */
    while (!done) {
        isBasic = 0;
        intraCommName = "";
        switch (intraCommIdx) {
        case 0:
            *comm = MPI_COMM_WORLD;
            isBasic = 1;
            intraCommName = "MPI_COMM_WORLD";
            break;
        case 1:
            /* dup of world */
            merr = MPI_Comm_dup(MPI_COMM_WORLD, comm);
            if (merr)
                MTestPrintError(merr);
            intraCommName = "Dup of MPI_COMM_WORLD";
            break;
        case 2:
            /* reverse ranks */
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_split(MPI_COMM_WORLD, 0, size - rank, comm);
            if (merr)
                MTestPrintError(merr);
            intraCommName = "Rank reverse of MPI_COMM_WORLD";
            break;
        case 3:
            /* subset of world, with reversed ranks */
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_split(MPI_COMM_WORLD, ((rank < size / 2) ? 1 : MPI_UNDEFINED),
                                  size - rank, comm);
            if (merr)
                MTestPrintError(merr);
            intraCommName = "Rank reverse of half of MPI_COMM_WORLD";
            break;
        case 4:
            *comm = MPI_COMM_SELF;
            isBasic = 1;
            intraCommName = "MPI_COMM_SELF";
            break;
        case 5:
            {
                /* Dup of the world using MPI_Intercomm_merge */
                int rleader, isLeft;
                MPI_Comm local_comm, inter_comm;
                MPI_Comm_size(MPI_COMM_WORLD, &size);
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                if (size > 1) {
                    merr = MPI_Comm_split(MPI_COMM_WORLD, (rank < size / 2), rank, &local_comm);
                    if (merr)
                        MTestPrintError(merr);
                    if (rank == 0) {
                        rleader = size / 2;
                    }
                    else if (rank == size / 2) {
                        rleader = 0;
                    }
                    else {
                        rleader = -1;
                    }
                    isLeft = rank < size / 2;
                    merr =
                        MPI_Intercomm_create(local_comm, 0, MPI_COMM_WORLD, rleader, 99,
                                             &inter_comm);
                    if (merr)
                        MTestPrintError(merr);
                    merr = MPI_Intercomm_merge(inter_comm, isLeft, comm);
                    if (merr)
                        MTestPrintError(merr);
                    MPI_Comm_free(&inter_comm);
                    MPI_Comm_free(&local_comm);
                    intraCommName = "Dup of WORLD created by MPI_Intercomm_merge";
                }
                else {
                    *comm = MPI_COMM_NULL;
                }
            }
            break;
        case 6:
            {
#if MTEST_HAVE_MIN_MPI_VERSION(3,0)
                /* Even of the world using MPI_Comm_create_group */
                int i;
                MPI_Group world_group, even_group;
                int *excl = NULL;

                MPI_Comm_size(MPI_COMM_WORLD, &size);
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                if (allowSmaller && (size + 1) / 2 >= min_size) {
                    /* exclude the odd ranks */
                    excl = malloc((size / 2) * sizeof(int));
                    for (i = 0; i < size / 2; i++)
                        excl[i] = (2 * i) + 1;

                    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
                    MPI_Group_excl(world_group, size / 2, excl, &even_group);
                    MPI_Group_free(&world_group);
                    free(excl);

                    if (rank % 2 == 0) {
                        /* Even processes create a comm. for themselves */
                        MPI_Comm_create_group(MPI_COMM_WORLD, even_group, 0, comm);
                        intraCommName = "Even of WORLD created by MPI_Comm_create_group";
                    }
                    else {
                        *comm = MPI_COMM_NULL;
                    }

                    MPI_Group_free(&even_group);
                }
                else {
                    *comm = MPI_COMM_NULL;
                }
#else
                *comm = MPI_COMM_NULL;
#endif
            }
            break;
        case 7:
            {
                /* High half of the world using MPI_Comm_create */
                int ranges[1][3];
                MPI_Group world_group, high_group;
                MPI_Comm_size(MPI_COMM_WORLD, &size);
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                ranges[0][0] = size / 2;
                ranges[0][1] = size - 1;
                ranges[0][2] = 1;

                if (allowSmaller && (size + 1) / 2 >= min_size) {
                    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
                    merr = MPI_Group_range_incl(world_group, 1, ranges, &high_group);
                    if (merr)
                        MTestPrintError(merr);
                    merr = MPI_Comm_create(MPI_COMM_WORLD, high_group, comm);
                    if (merr)
                        MTestPrintError(merr);
                    MPI_Group_free(&world_group);
                    MPI_Group_free(&high_group);
                    intraCommName = "High half of WORLD created by MPI_Comm_create";
                }
                else {
                    *comm = MPI_COMM_NULL;
                }
            }
            break;
            /* These next cases are communicators that include some
             * but not all of the processes */
        case 8:
        case 9:
        case 10:
        case 11:
            {
                int newsize;
                merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
                if (merr)
                    MTestPrintError(merr);
                newsize = size - (intraCommIdx - 7);

                if (allowSmaller && newsize >= min_size) {
                    merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                    if (merr)
                        MTestPrintError(merr);
                    merr = MPI_Comm_split(MPI_COMM_WORLD, rank < newsize, rank, comm);
                    if (merr)
                        MTestPrintError(merr);
                    if (rank >= newsize) {
                        merr = MPI_Comm_free(comm);
                        if (merr)
                            MTestPrintError(merr);
                        *comm = MPI_COMM_NULL;
                    }
                    else {
                        intraCommName = "Split of WORLD";
                    }
                }
                else {
                    /* Act like default */
                    *comm = MPI_COMM_NULL;
                    intraCommIdx = -1;
                }
            }
            break;

            /* Other ideas: dup of self, cart comm, graph comm */
        default:
            *comm = MPI_COMM_NULL;
            intraCommIdx = -1;
            break;
        }

        if (*comm != MPI_COMM_NULL) {
            merr = MPI_Comm_size(*comm, &size);
            if (merr)
                MTestPrintError(merr);
            if (size >= min_size)
                done = 1;
        }
        else {
            intraCommName = "MPI_COMM_NULL";
            isBasic = 1;
            done = 1;
        }

        /* we are only done if all processes are done */
        MPI_Allreduce(MPI_IN_PLACE, &done, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

        /* Advance the comm index whether we are done or not, otherwise we could
         * spin forever trying to allocate a too-small communicator over and
         * over again. */
        intraCommIdx++;

        if (!done && !isBasic && *comm != MPI_COMM_NULL) {
            /* avoid leaking communicators */
            merr = MPI_Comm_free(comm);
            if (merr)
                MTestPrintError(merr);
        }
    }

    return intraCommIdx;
}

/*
 * Get an intracommunicator with at least min_size members.
 */
int MTestGetIntracomm(MPI_Comm * comm, int min_size)
{
    return MTestGetIntracommGeneral(comm, min_size, 0);
}

/* Return the name of an intra communicator */
const char *MTestGetIntracommName(void)
{
    return intraCommName;
}

/*
 * Return an intercomm; set isLeftGroup to 1 if the calling process is
 * a member of the "left" group.
 */
int MTestGetIntercomm(MPI_Comm * comm, int *isLeftGroup, int min_size)
{
    int size, rank, remsize, merr;
    int done = 0;
    MPI_Comm mcomm = MPI_COMM_NULL;
    MPI_Comm mcomm2 = MPI_COMM_NULL;
    int rleader;

    /* The while loop allows us to skip communicators that are too small.
     * MPI_COMM_NULL is always considered large enough.  The size is
     * the sum of the sizes of the local and remote groups */
    while (!done) {
        *comm = MPI_COMM_NULL;
        *isLeftGroup = 0;
        interCommName = "MPI_COMM_NULL";

        switch (interCommIdx) {
        case 0:
            /* Split comm world in half */
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            if (size > 1) {
                merr = MPI_Comm_split(MPI_COMM_WORLD, (rank < size / 2), rank, &mcomm);
                if (merr)
                    MTestPrintError(merr);
                if (rank == 0) {
                    rleader = size / 2;
                }
                else if (rank == size / 2) {
                    rleader = 0;
                }
                else {
                    /* Remote leader is signficant only for the processes
                     * designated local leaders */
                    rleader = -1;
                }
                *isLeftGroup = rank < size / 2;
                merr = MPI_Intercomm_create(mcomm, 0, MPI_COMM_WORLD, rleader, 12345, comm);
                if (merr)
                    MTestPrintError(merr);
                interCommName = "Intercomm by splitting MPI_COMM_WORLD";
            }
            else
                *comm = MPI_COMM_NULL;
            break;
        case 1:
            /* Split comm world in to 1 and the rest */
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            if (size > 1) {
                merr = MPI_Comm_split(MPI_COMM_WORLD, rank == 0, rank, &mcomm);
                if (merr)
                    MTestPrintError(merr);
                if (rank == 0) {
                    rleader = 1;
                }
                else if (rank == 1) {
                    rleader = 0;
                }
                else {
                    /* Remote leader is signficant only for the processes
                     * designated local leaders */
                    rleader = -1;
                }
                *isLeftGroup = rank == 0;
                merr = MPI_Intercomm_create(mcomm, 0, MPI_COMM_WORLD, rleader, 12346, comm);
                if (merr)
                    MTestPrintError(merr);
                interCommName = "Intercomm by splitting MPI_COMM_WORLD into 1, rest";
            }
            else
                *comm = MPI_COMM_NULL;
            break;

        case 2:
            /* Split comm world in to 2 and the rest */
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            if (size > 3) {
                merr = MPI_Comm_split(MPI_COMM_WORLD, rank < 2, rank, &mcomm);
                if (merr)
                    MTestPrintError(merr);
                if (rank == 0) {
                    rleader = 2;
                }
                else if (rank == 2) {
                    rleader = 0;
                }
                else {
                    /* Remote leader is signficant only for the processes
                     * designated local leaders */
                    rleader = -1;
                }
                *isLeftGroup = rank < 2;
                merr = MPI_Intercomm_create(mcomm, 0, MPI_COMM_WORLD, rleader, 12347, comm);
                if (merr)
                    MTestPrintError(merr);
                interCommName = "Intercomm by splitting MPI_COMM_WORLD into 2, rest";
            }
            else
                *comm = MPI_COMM_NULL;
            break;

        case 3:
            /* Split comm world in half, then dup */
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            if (size > 1) {
                merr = MPI_Comm_split(MPI_COMM_WORLD, (rank < size / 2), rank, &mcomm);
                if (merr)
                    MTestPrintError(merr);
                if (rank == 0) {
                    rleader = size / 2;
                }
                else if (rank == size / 2) {
                    rleader = 0;
                }
                else {
                    /* Remote leader is signficant only for the processes
                     * designated local leaders */
                    rleader = -1;
                }
                *isLeftGroup = rank < size / 2;
                merr = MPI_Intercomm_create(mcomm, 0, MPI_COMM_WORLD, rleader, 12345, comm);
                if (merr)
                    MTestPrintError(merr);
                /* avoid leaking after assignment below */
                merr = MPI_Comm_free(&mcomm);
                if (merr)
                    MTestPrintError(merr);

                /* now dup, some bugs only occur for dup's of intercomms */
                mcomm = *comm;
                merr = MPI_Comm_dup(mcomm, comm);
                if (merr)
                    MTestPrintError(merr);
                interCommName = "Intercomm by splitting MPI_COMM_WORLD then dup'ing";
            }
            else
                *comm = MPI_COMM_NULL;
            break;

        case 4:
            /* Split comm world in half, form intercomm, then split that intercomm */
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            if (size > 1) {
                merr = MPI_Comm_split(MPI_COMM_WORLD, (rank < size / 2), rank, &mcomm);
                if (merr)
                    MTestPrintError(merr);
                if (rank == 0) {
                    rleader = size / 2;
                }
                else if (rank == size / 2) {
                    rleader = 0;
                }
                else {
                    /* Remote leader is signficant only for the processes
                     * designated local leaders */
                    rleader = -1;
                }
                *isLeftGroup = rank < size / 2;
                merr = MPI_Intercomm_create(mcomm, 0, MPI_COMM_WORLD, rleader, 12345, comm);
                if (merr)
                    MTestPrintError(merr);
                /* avoid leaking after assignment below */
                merr = MPI_Comm_free(&mcomm);
                if (merr)
                    MTestPrintError(merr);

                /* now split, some bugs only occur for splits of intercomms */
                mcomm = *comm;
                merr = MPI_Comm_rank(mcomm, &rank);
                if (merr)
                    MTestPrintError(merr);
                /* this split is effectively a dup but tests the split code paths */
                merr = MPI_Comm_split(mcomm, 0, rank, comm);
                if (merr)
                    MTestPrintError(merr);
                interCommName = "Intercomm by splitting MPI_COMM_WORLD then then splitting again";
            }
            else
                *comm = MPI_COMM_NULL;
            break;

        case 5:
            /* split comm world in half discarding rank 0 on the "left"
             * communicator, then form them into an intercommunicator */
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            if (size >= 4) {
                int color = (rank < size / 2 ? 0 : 1);
                if (rank == 0)
                    color = MPI_UNDEFINED;

                merr = MPI_Comm_split(MPI_COMM_WORLD, color, rank, &mcomm);
                if (merr)
                    MTestPrintError(merr);

                if (rank == 1) {
                    rleader = size / 2;
                }
                else if (rank == (size / 2)) {
                    rleader = 1;
                }
                else {
                    /* Remote leader is signficant only for the processes
                     * designated local leaders */
                    rleader = -1;
                }
                *isLeftGroup = rank < size / 2;
                if (rank != 0) {        /* 0's mcomm is MPI_COMM_NULL */
                    merr = MPI_Intercomm_create(mcomm, 0, MPI_COMM_WORLD, rleader, 12345, comm);
                    if (merr)
                        MTestPrintError(merr);
                }
                interCommName =
                    "Intercomm by splitting MPI_COMM_WORLD (discarding rank 0 in the left group) then MPI_Intercomm_create'ing";
            }
            else {
                *comm = MPI_COMM_NULL;
            }
            break;

        case 6:
            /* Split comm world in half then form them into an
             * intercommunicator.  Then discard rank 0 from each group of the
             * intercomm via MPI_Comm_create. */
            merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (merr)
                MTestPrintError(merr);
            if (size >= 4) {
                MPI_Group oldgroup, newgroup;
                int ranks[1];
                int color = (rank < size / 2 ? 0 : 1);

                merr = MPI_Comm_split(MPI_COMM_WORLD, color, rank, &mcomm);
                if (merr)
                    MTestPrintError(merr);

                if (rank == 0) {
                    rleader = size / 2;
                }
                else if (rank == (size / 2)) {
                    rleader = 0;
                }
                else {
                    /* Remote leader is signficant only for the processes
                     * designated local leaders */
                    rleader = -1;
                }
                *isLeftGroup = rank < size / 2;
                merr = MPI_Intercomm_create(mcomm, 0, MPI_COMM_WORLD, rleader, 12345, &mcomm2);
                if (merr)
                    MTestPrintError(merr);

                /* We have an intercomm between the two halves of comm world. Now create
                 * a new intercomm that removes rank 0 on each side. */
                merr = MPI_Comm_group(mcomm2, &oldgroup);
                if (merr)
                    MTestPrintError(merr);
                ranks[0] = 0;
                merr = MPI_Group_excl(oldgroup, 1, ranks, &newgroup);
                if (merr)
                    MTestPrintError(merr);
                merr = MPI_Comm_create(mcomm2, newgroup, comm);
                if (merr)
                    MTestPrintError(merr);

                merr = MPI_Group_free(&oldgroup);
                if (merr)
                    MTestPrintError(merr);
                merr = MPI_Group_free(&newgroup);
                if (merr)
                    MTestPrintError(merr);

                interCommName =
                    "Intercomm by splitting MPI_COMM_WORLD then discarding 0 ranks with MPI_Comm_create";
            }
            else {
                *comm = MPI_COMM_NULL;
            }
            break;

        default:
            *comm = MPI_COMM_NULL;
            interCommIdx = -1;
            break;
        }

        if (*comm != MPI_COMM_NULL) {
            merr = MPI_Comm_size(*comm, &size);
            if (merr)
                MTestPrintError(merr);
            merr = MPI_Comm_remote_size(*comm, &remsize);
            if (merr)
                MTestPrintError(merr);
            if (size + remsize >= min_size)
                done = 1;
        }
        else {
            interCommName = "MPI_COMM_NULL";
            done = 1;
        }

        /* we are only done if all processes are done */
        MPI_Allreduce(MPI_IN_PLACE, &done, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

        /* Advance the comm index whether we are done or not, otherwise we could
         * spin forever trying to allocate a too-small communicator over and
         * over again. */
        interCommIdx++;

        if (!done && *comm != MPI_COMM_NULL) {
            /* avoid leaking communicators */
            merr = MPI_Comm_free(comm);
            if (merr)
                MTestPrintError(merr);
        }

        /* cleanup for common temp objects */
        if (mcomm != MPI_COMM_NULL) {
            merr = MPI_Comm_free(&mcomm);
            if (merr)
                MTestPrintError(merr);
        }
        if (mcomm2 != MPI_COMM_NULL) {
            merr = MPI_Comm_free(&mcomm2);
            if (merr)
                MTestPrintError(merr);
        }
    }

    return interCommIdx;
}

int MTestTestIntercomm(MPI_Comm comm)
{
    int local_size, remote_size, rank, **bufs, *bufmem, rbuf[2], j;
    int errs = 0, wrank, nsize;
    char commname[MPI_MAX_OBJECT_NAME + 1];
    MPI_Request *reqs;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(comm, &local_size);
    MPI_Comm_remote_size(comm, &remote_size);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_get_name(comm, commname, &nsize);

    MTestPrintfMsg(1, "Testing communication on intercomm '%s', remote_size=%d\n",
                   commname, remote_size);

    reqs = (MPI_Request *) malloc(remote_size * sizeof(MPI_Request));
    if (!reqs) {
        printf("[%d] Unable to allocated %d requests for testing intercomm %s\n",
               wrank, remote_size, commname);
        errs++;
        return errs;
    }
    bufs = (int **) malloc(remote_size * sizeof(int *));
    if (!bufs) {
        printf("[%d] Unable to allocated %d int pointers for testing intercomm %s\n",
               wrank, remote_size, commname);
        errs++;
        return errs;
    }
    bufmem = (int *) malloc(remote_size * 2 * sizeof(int));
    if (!bufmem) {
        printf("[%d] Unable to allocated %d int data for testing intercomm %s\n",
               wrank, 2 * remote_size, commname);
        errs++;
        return errs;
    }

    /* Each process sends a message containing its own rank and the
     * rank of the destination with a nonblocking send.  Because we're using
     * nonblocking sends, we need to use different buffers for each isend */
    /* NOTE: the send buffer access restriction was relaxed in MPI-2.2, although
     * it doesn't really hurt to keep separate buffers for our purposes */
    for (j = 0; j < remote_size; j++) {
        bufs[j] = &bufmem[2 * j];
        bufs[j][0] = rank;
        bufs[j][1] = j;
        MPI_Isend(bufs[j], 2, MPI_INT, j, 0, comm, &reqs[j]);
    }
    MTestPrintfMsg(2, "isends posted, about to recv\n");

    for (j = 0; j < remote_size; j++) {
        MPI_Recv(rbuf, 2, MPI_INT, j, 0, comm, MPI_STATUS_IGNORE);
        if (rbuf[0] != j) {
            printf("[%d] Expected rank %d but saw %d in %s\n", wrank, j, rbuf[0], commname);
            errs++;
        }
        if (rbuf[1] != rank) {
            printf("[%d] Expected target rank %d but saw %d from %d in %s\n",
                   wrank, rank, rbuf[1], j, commname);
            errs++;
        }
    }
    if (errs)
        fflush(stdout);

    MTestPrintfMsg(2, "my recvs completed, about to waitall\n");
    MPI_Waitall(remote_size, reqs, MPI_STATUSES_IGNORE);

    free(reqs);
    free(bufs);
    free(bufmem);

    return errs;
}

int MTestTestIntracomm(MPI_Comm comm)
{
    int i, errs = 0;
    int size;
    int in[16], out[16], sol[16];

    MPI_Comm_size(comm, &size);

    /* Set input, output and sol-values */
    for (i = 0; i < 16; i++) {
        in[i] = i;
        out[i] = 0;
        sol[i] = i * size;
    }
    MPI_Allreduce(in, out, 16, MPI_INT, MPI_SUM, comm);

    /* Test results */
    for (i = 0; i < 16; i++) {
        if (sol[i] != out[i])
            errs++;
    }

    return errs;
}

int MTestTestComm(MPI_Comm comm)
{
    int is_inter;

    if (comm == MPI_COMM_NULL)
        return 0;

    MPI_Comm_test_inter(comm, &is_inter);

    if (is_inter)
        return MTestTestIntercomm(comm);
    else
        return MTestTestIntracomm(comm);
}

/* Return the name of an intercommunicator */
const char *MTestGetIntercommName(void)
{
    return interCommName;
}

/* Get a communicator of a given minimum size.  Both intra and inter
   communicators are provided */
int MTestGetComm(MPI_Comm * comm, int min_size)
{
    int idx = 0;
    static int getinter = 0;

    if (!getinter) {
        idx = MTestGetIntracomm(comm, min_size);
        if (idx == 0) {
            getinter = 1;
        }
    }
    if (getinter) {
        int isLeft;
        idx = MTestGetIntercomm(comm, &isLeft, min_size);
        if (idx == 0) {
            getinter = 0;
        }
    }

    return idx;
}

/* Free a communicator.  It may be called with a predefined communicator
 or MPI_COMM_NULL */
void MTestFreeComm(MPI_Comm * comm)
{
    int merr;
    if (*comm != MPI_COMM_WORLD && *comm != MPI_COMM_SELF && *comm != MPI_COMM_NULL) {
        merr = MPI_Comm_free(comm);
        if (merr)
            MTestPrintError(merr);
    }
}

/* ------------------------------------------------------------------------ */
void MTestPrintError(int errcode)
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];

    MPI_Error_class(errcode, &errclass);
    MPI_Error_string(errcode, string, &slen);
    printf("Error class %d (%s)\n", errclass, string);
    fflush(stdout);
}

void MTestPrintErrorMsg(const char msg[], int errcode)
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];

    MPI_Error_class(errcode, &errclass);
    MPI_Error_string(errcode, string, &slen);
    printf("%s: Error class %d (%s)\n", msg, errclass, string);
    fflush(stdout);
}

/* ------------------------------------------------------------------------ */
/*
 If verbose output is selected and the level is at least that of the
 value of the verbose flag, then perform printf(format, ...);
 */
void MTestPrintfMsg(int level, const char format[], ...)
{
    va_list list;
    int n;

    if (verbose && level >= verbose) {
        va_start(list, format);
        n = vprintf(format, list);
        va_end(list);
        fflush(stdout);
    }
}

/* Fatal error.  Report and exit */
void MTestError(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

/* ------------------------------------------------------------------------ */
static void MTestResourceSummary(FILE * fp)
{
#ifdef HAVE_GETRUSAGE
    struct rusage ru;
    static int pfThreshold = -2;
    int doOutput = 1;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        /* There is an option to generate output only when a resource
         * exceeds a threshold.  To date, only page faults supported. */
        if (pfThreshold == -2) {
            char *p = getenv("MPITEST_RUSAGE_PF");
            pfThreshold = -1;
            if (p) {
                pfThreshold = strtol(p, 0, 0);
            }
        }
        if (pfThreshold > 0) {
            doOutput = ru.ru_minflt > pfThreshold;
        }
        if (doOutput) {
            /* Cast values to long in case some system has defined them
             * as another integer type */
            fprintf(fp, "RUSAGE: max resident set = %ldKB\n", (long) ru.ru_maxrss);
            fprintf(fp, "RUSAGE: page faults = %ld : %ld\n",
                    (long) ru.ru_minflt, (long) ru.ru_majflt);
            /* Not every Unix provides useful information for the xxrss fields */
            fprintf(fp, "RUSAGE: memory in text/data/stack = %ld : %ld : %ld\n",
                    (long) ru.ru_ixrss, (long) ru.ru_idrss, (long) ru.ru_isrss);
            fprintf(fp, "RUSAGE: I/O in and out = %ld : %ld\n",
                    (long) ru.ru_inblock, (long) ru.ru_oublock);
            fprintf(fp, "RUSAGE: context switch = %ld : %ld\n",
                    (long) ru.ru_nvcsw, (long) ru.ru_nivcsw);
        }
    }
    else {
        fprintf(fp, "RUSAGE: return error %d\n", errno);
    }
#endif
}

/* ------------------------------------------------------------------------ */
#ifdef HAVE_MPI_WIN_CREATE
/*
 * Create MPI Windows
 */
static int win_index = 0;
static const char *winName;
/* Use an attribute to remember the type of memory allocation (static,
   malloc, or MPI_Alloc_mem) */
static int mem_keyval = MPI_KEYVAL_INVALID;
int MTestGetWin(MPI_Win * win, int mustBePassive)
{
    static char actbuf[1024];
    static char *pasbuf;
    char *buf;
    int n, rank, merr;
    MPI_Info info;

    if (mem_keyval == MPI_KEYVAL_INVALID) {
        /* Create the keyval */
        merr = MPI_Win_create_keyval(MPI_WIN_NULL_COPY_FN, MPI_WIN_NULL_DELETE_FN, &mem_keyval, 0);
        if (merr)
            MTestPrintError(merr);

    }

    switch (win_index) {
    case 0:
        /* Active target window */
        merr = MPI_Win_create(actbuf, 1024, 1, MPI_INFO_NULL, MPI_COMM_WORLD, win);
        if (merr)
            MTestPrintError(merr);
        winName = "active-window";
        merr = MPI_Win_set_attr(*win, mem_keyval, (void *) 0);
        if (merr)
            MTestPrintError(merr);
        break;
    case 1:
        /* Passive target window */
        merr = MPI_Alloc_mem(1024, MPI_INFO_NULL, &pasbuf);
        if (merr)
            MTestPrintError(merr);
        merr = MPI_Win_create(pasbuf, 1024, 1, MPI_INFO_NULL, MPI_COMM_WORLD, win);
        if (merr)
            MTestPrintError(merr);
        winName = "passive-window";
        merr = MPI_Win_set_attr(*win, mem_keyval, (void *) 2);
        if (merr)
            MTestPrintError(merr);
        break;
    case 2:
        /* Active target; all windows different sizes */
        merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (merr)
            MTestPrintError(merr);
        n = rank * 64;
        if (n)
            buf = (char *) malloc(n);
        else
            buf = 0;
        merr = MPI_Win_create(buf, n, 1, MPI_INFO_NULL, MPI_COMM_WORLD, win);
        if (merr)
            MTestPrintError(merr);
        winName = "active-all-different-win";
        merr = MPI_Win_set_attr(*win, mem_keyval, (void *) 1);
        if (merr)
            MTestPrintError(merr);
        break;
    case 3:
        /* Active target, no locks set */
        merr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (merr)
            MTestPrintError(merr);
        n = rank * 64;
        if (n)
            buf = (char *) malloc(n);
        else
            buf = 0;
        merr = MPI_Info_create(&info);
        if (merr)
            MTestPrintError(merr);
        merr = MPI_Info_set(info, (char *) "nolocks", (char *) "true");
        if (merr)
            MTestPrintError(merr);
        merr = MPI_Win_create(buf, n, 1, info, MPI_COMM_WORLD, win);
        if (merr)
            MTestPrintError(merr);
        merr = MPI_Info_free(&info);
        if (merr)
            MTestPrintError(merr);
        winName = "active-nolocks-all-different-win";
        merr = MPI_Win_set_attr(*win, mem_keyval, (void *) 1);
        if (merr)
            MTestPrintError(merr);
        break;
    default:
        win_index = -1;
    }
    win_index++;
    return win_index;
}

/* Return a pointer to the name associated with a window object */
const char *MTestGetWinName(void)
{
    return winName;
}

/* Free the storage associated with a window object */
void MTestFreeWin(MPI_Win * win)
{
    void *addr;
    int flag, merr;

    merr = MPI_Win_get_attr(*win, MPI_WIN_BASE, &addr, &flag);
    if (merr)
        MTestPrintError(merr);
    if (!flag) {
        MTestError("Could not get WIN_BASE from window");
    }
    if (addr) {
        void *val;
        merr = MPI_Win_get_attr(*win, mem_keyval, &val, &flag);
        if (merr)
            MTestPrintError(merr);
        if (flag) {
            if (val == (void *) 1) {
                free(addr);
            }
            else if (val == (void *) 2) {
                merr = MPI_Free_mem(addr);
                if (merr)
                    MTestPrintError(merr);
            }
            /* if val == (void *)0, then static data that must not be freed */
        }
    }
    merr = MPI_Win_free(win);
    if (merr)
        MTestPrintError(merr);
}

static void MTestRMACleanup(void)
{
    if (mem_keyval != MPI_KEYVAL_INVALID) {
        MPI_Win_free_keyval(&mem_keyval);
    }
}
#else
static void MTestRMACleanup(void)
{
}
#endif

/* ------------------------------------------------------------------------ */
/* This function determines if it is possible to spawn addition MPI
 * processes using MPI_COMM_SPAWN and MPI_COMM_SPAWN_MULTIPLE.
 *
 * It sets the can_spawn value to one of the following:
 * 1  = yes, additional processes can be spawned
 * 0  = no, MPI_UNIVERSE_SIZE <= the size of MPI_COMM_WORLD
 * -1 = it is unknown whether or not processes can be spawned
 *      due to errors in the necessary query functions
 *
 */
int MTestSpawnPossible(int *can_spawn)
{
    int errs = 0;

    void *v = NULL;
    int flag = -1;
    int vval = -1;
    int rc;

    rc = MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &v, &flag);
    if (rc != MPI_SUCCESS) {
        /* MPI_UNIVERSE_SIZE keyval missing from MPI_COMM_WORLD attributes */
        *can_spawn = -1;
        errs++;
    }
    else {
        /* MPI_UNIVERSE_SIZE need not be set */
        if (flag) {

            int size = -1;
            rc = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (rc != MPI_SUCCESS) {
                /* MPI_Comm_size failed for MPI_COMM_WORLD */
                *can_spawn = -1;
                errs++;
            }

            vval = *(int *) v;
            if (vval <= size) {
                /* no additional processes can be spawned */
                *can_spawn = 0;
            }
            else {
                *can_spawn = 1;
            }
        }
        else {
            /* No attribute associated with key MPI_UNIVERSE_SIZE of MPI_COMM_WORLD */
            *can_spawn = -1;
        }
    }
    return errs;
}

/* ------------------------------------------------------------------------ */
