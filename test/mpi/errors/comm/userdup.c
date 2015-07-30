/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

/* Check that user-define error codes and classes are correctly handled by
   the attribute copy routines.

   Note that this behavior is not required by the MPI specification
   but is a quality of implementation issues - users will expect
   to be able to control the class and code that comes back from
   MPI_Comm_dup (and MPI_Comm_free) in this case.
*/

void abortMsg(const char *, int);
int copybomb_fn(MPI_Comm, int, void *, void *, void *, int *);

static int myErrClass, myErrCode;
static int nCall = 0;

void abortMsg(const char *str, int code)
{
    char msg[MPI_MAX_ERROR_STRING];
    int class, resultLen;

    MPI_Error_class(code, &class);
    MPI_Error_string(code, msg, &resultLen);
    fprintf(stderr, "%s: errcode = %d, class = %d, msg = %s\n", str, code, class, msg);
    MPI_Abort(MPI_COMM_WORLD, code);
}

int copybomb_fn(MPI_Comm oldcomm, int keyval, void *extra_state,
                void *attribute_val_in, void *attribute_val_out, int *flag)
{
    int err;
    /* We always fail to copy */
    *flag = 1;

    /* Return either the class (as a code) or the code */
    if (nCall == 0)
        err = myErrClass;
    else
        err = myErrCode;
    nCall++;
    return err;
}


int main(int argc, char *argv[])
{
    MPI_Comm dupWorld, dup2;
    int myRank, key, err, errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* Create my error class and code */
    MPI_Add_error_class(&myErrClass);
    MPI_Add_error_code(myErrClass, &myErrCode);
    MPI_Add_error_string(myErrClass, (char *) "My error class");
    MPI_Add_error_string(myErrCode, (char *) "My error code");

    MPI_Comm_dup(MPI_COMM_WORLD, &dupWorld);

    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    err = MPI_Comm_create_keyval(copybomb_fn, MPI_COMM_NULL_DELETE_FN, &key, NULL);
    if (err) {
        errs++;
        abortMsg("Comm_create_keyval", err);
    }

    err = MPI_Comm_set_attr(dupWorld, key, &myRank);
    if (err) {
        errs++;
        abortMsg("Comm_set_attr", err);
    }

    err = MPI_Comm_dup(dupWorld, &dup2);
    if (err == MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "Comm_dup did not fail\n");
        MPI_Comm_free(&dup2);
    }
    else {
        int eclass, resultLen;
        char msg[MPI_MAX_ERROR_STRING];
        /* Check for expected error class */
        MPI_Error_class(err, &eclass);
        if (eclass != myErrClass) {
            errs++;
            fprintf(stderr, "Unexpected error class = %d, expected user-defined class %d\n", eclass,
                    myErrClass);
        }
        else {
            MPI_Error_string(err, msg, &resultLen);
            if (strcmp(msg, "My error class") != 0) {
                errs++;
                fprintf(stderr, "Unexpected error string %s\n", msg);
            }
        }
    }

    err = MPI_Comm_dup(dupWorld, &dup2);
    if (err == MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "Comm_dup did not fail (2)\n");
        MPI_Comm_free(&dup2);
    }
    else {
        int eclass, resultLen;
        char msg[MPI_MAX_ERROR_STRING];
        /* Check for expected error class */
        MPI_Error_class(err, &eclass);
        if (eclass != myErrClass) {
            errs++;
            fprintf(stderr, "Unexpected error class = %d, expected user-defined class %d\n", eclass,
                    myErrClass);
        }
        if (err != myErrCode) {
            errs++;
            fprintf(stderr, "Unexpected error code = %d, expected user-defined code %d\n", err,
                    myErrCode);
        }
        else {
            MPI_Error_string(err, msg, &resultLen);
            if (strcmp(msg, "My error code") != 0) {
                errs++;
                fprintf(stderr, "Unexpected error string %s, expected user-defined error string\n",
                        msg);
            }
        }
    }

    MPI_Comm_free(&dupWorld);
    MPI_Comm_free_keyval(&key);

    MTest_Finalize(errs);

    MPI_Finalize();
    return 0;
}
