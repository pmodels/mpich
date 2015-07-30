/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*

  Exercise attribute routines.
  This version checks for correct behavior of the copy and delete functions
  on an attribute, particularly the correct behavior when the routine returns
  failure.

 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

int test_attrs(void);
void abort_msg(const char *, int);
int copybomb_fn(MPI_Datatype, int, void *, void *, void *, int *);
int deletebomb_fn(MPI_Datatype, int, void *, void *);

int main(int argc, char **argv)
{
    int errs;
    MTest_Init(&argc, &argv);
    errs = test_attrs();
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}

/*
 * MPI 1.2 Clarification: Clarification of Error Behavior of
 *                        Attribute Callback Functions
 * Any return value other than MPI_SUCCESS is erroneous.  The specific value
 * returned to the user is undefined (other than it can't be MPI_SUCCESS).
 * Proposals to specify particular values (e.g., user's value) failed.
 */
/* Return an error as the value */
int copybomb_fn(MPI_Datatype oldtype, int keyval, void *extra_state,
                void *attribute_val_in, void *attribute_val_out, int *flag)
{
    /* Note that if (sizeof(int) < sizeof(void *), just setting the int
     * part of attribute_val_out may leave some dirty bits
     */
    *flag = 1;
    return MPI_ERR_OTHER;
}

/* Set delete flag to 1 to allow the attribute to be deleted */
static int delete_flag = 0;
static int deleteCalled = 0;

int deletebomb_fn(MPI_Datatype type, int keyval, void *attribute_val, void *extra_state)
{
    deleteCalled++;
    if (delete_flag)
        return MPI_SUCCESS;
    return MPI_ERR_OTHER;
}

void abort_msg(const char *str, int code)
{
    fprintf(stderr, "%s, err = %d\n", str, code);
    MPI_Abort(MPI_COMM_WORLD, code);
}

int test_attrs(void)
{
    MPI_Datatype dup_type, d2;
    int world_rank, world_size, key_1;
    int err, errs = 0;
    MPI_Aint value;

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
#ifdef DEBUG
    if (world_rank == 0) {
        printf("*** Attribute copy/delete return codes ***\n");
    }
#endif


    MPI_Type_dup(MPI_DOUBLE, &dup_type);

    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    value = -11;
    if ((err = MPI_Type_create_keyval(copybomb_fn, deletebomb_fn, &key_1, &value)))
        abort_msg("Keyval_create", err);

    err = MPI_Type_set_attr(dup_type, key_1, (void *) (MPI_Aint) world_rank);
    if (err) {
        errs++;
        printf("Error with first put\n");
    }

    err = MPI_Type_set_attr(dup_type, key_1, (void *) (MPI_Aint) (2 * world_rank));
    if (err == MPI_SUCCESS) {
        errs++;
        printf("delete function return code was MPI_SUCCESS in put\n");
    }

    /* Because the attribute delete function should fail, the attribute
     * should *not be removed* */
    err = MPI_Type_delete_attr(dup_type, key_1);
    if (err == MPI_SUCCESS) {
        errs++;
        printf("delete function return code was MPI_SUCCESS in delete\n");
    }

    err = MPI_Type_dup(dup_type, &d2);
    if (err == MPI_SUCCESS) {
        errs++;
        printf("copy function return code was MPI_SUCCESS in dup\n");
    }
#ifndef USE_STRICT_MPI
    /* Another interpretation is to leave d2 unchanged on error */
    if (err && d2 != MPI_DATATYPE_NULL) {
        errs++;
        printf("dup did not return MPI_DATATYPE_NULL on error\n");
    }
#endif

    delete_flag = 1;
    deleteCalled = 0;
    if (d2 != MPI_DATATYPE_NULL)
        MPI_Type_free(&d2);
    MPI_Type_free(&dup_type);
    if (deleteCalled == 0) {
        errs++;
        printf("Free of a datatype did not invoke the attribute delete routine\n");
    }
    MPI_Type_free_keyval(&key_1);

    return errs;
}
