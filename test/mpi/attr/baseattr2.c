/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

void MissingKeyval(int rc, const char keyname[]);

int main(int argc, char **argv)
{
    int errs = 0;
    int rc;
    void *v;
    int flag;
    int vval;
    int rank, size;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Set errors return so that we can provide better information
     * should a routine reject one of the attribute values */
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_TAG_UB, &v, &flag);
    if (rc) {
        MissingKeyval(rc, "MPI_TAG_UB");
        errs++;
    }
    else {
        if (!flag) {
            errs++;
            fprintf(stderr, "Could not get TAG_UB\n");
        }
        else {
            vval = *(int *) v;
            if (vval < 32767) {
                errs++;
                fprintf(stderr, "Got too-small value (%d) for TAG_UB\n", vval);
            }
        }
    }

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_HOST, &v, &flag);
    if (rc) {
        MissingKeyval(rc, "MPI_HOST");
        errs++;
    }
    else {
        if (!flag) {
            errs++;
            fprintf(stderr, "Could not get HOST\n");
        }
        else {
            vval = *(int *) v;
            if ((vval < 0 || vval >= size) && vval != MPI_PROC_NULL) {
                errs++;
                fprintf(stderr, "Got invalid value %d for HOST\n", vval);
            }
        }
    }

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_IO, &v, &flag);
    if (rc) {
        MissingKeyval(rc, "MPI_IO");
        errs++;
    }
    else {
        if (!flag) {
            errs++;
            fprintf(stderr, "Could not get IO\n");
        }
        else {
            vval = *(int *) v;
            if ((vval < 0 || vval >= size) && vval != MPI_ANY_SOURCE && vval != MPI_PROC_NULL) {
                errs++;
                fprintf(stderr, "Got invalid value %d for IO\n", vval);
            }
        }
    }

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL, &v, &flag);
    if (rc) {
        MissingKeyval(rc, "MPI_WTIME_IS_GLOBAL");
        errs++;
    }
    else {
        if (flag) {
            /* Wtime need not be set */
            vval = *(int *) v;
            if (vval < 0 || vval > 1) {
                errs++;
                fprintf(stderr, "Invalid value for WTIME_IS_GLOBAL (got %d)\n", vval);
            }
        }
    }

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_APPNUM, &v, &flag);
    if (rc) {
        MissingKeyval(rc, "MPI_APPNUM");
        errs++;
    }
    else {
        /* appnum need not be set */
        if (flag) {
            vval = *(int *) v;
            if (vval < 0) {
                errs++;
                fprintf(stderr, "MPI_APPNUM is defined as %d but must be nonnegative\n", vval);
            }
        }
    }

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &v, &flag);
    if (rc) {
        MissingKeyval(rc, "MPI_UNIVERSE_SIZE");
        errs++;
    }
    else {
        /* MPI_UNIVERSE_SIZE need not be set */
        if (flag) {
            vval = *(int *) v;
            if (vval < size) {
                errs++;
                fprintf(stderr, "MPI_UNIVERSE_SIZE = %d, less than comm world (%d)\n", vval, size);
            }
        }
    }

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_LASTUSEDCODE, &v, &flag);
    if (rc) {
        MissingKeyval(rc, "MPI_LASTUSEDCODE");
        errs++;
    }
    else {
        /* Last used code must be defined and >= MPI_ERR_LASTCODE */
        if (flag) {
            vval = *(int *) v;
            if (vval < MPI_ERR_LASTCODE) {
                errs++;
                fprintf(stderr,
                        "MPI_LASTUSEDCODE points to an integer (%d) smaller than MPI_ERR_LASTCODE (%d)\n",
                        vval, MPI_ERR_LASTCODE);
            }
        }
        else {
            errs++;
            fprintf(stderr, "MPI_LASTUSECODE is not defined\n");
        }
    }

    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}

void MissingKeyval(int errcode, const char keyname[])
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];

    MPI_Error_class(errcode, &errclass);
    MPI_Error_string(errcode, string, &slen);
    printf("For key %s: Error class %d (%s)\n", keyname, errclass, string);
    fflush(stdout);
}
