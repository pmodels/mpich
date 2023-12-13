/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static int attr_dup_fn(void *attr_in, void *attr_out, int *flag)
{
    /* Set attr_out, the flag and return success */
    (*(void **) attr_out) = attr_in;
    (*flag) = 1;
    return (MPI_SUCCESS);
}

/*D

MPI_DUP_FN - A function to simple-mindedly copy attributes

D*/
int MPIR_Dup_fn(MPI_Comm comm ATTRIBUTE((unused)),
                int keyval ATTRIBUTE((unused)),
                void *extra_state ATTRIBUTE((unused)), void *attr_in, void *attr_out, int *flag)
{
    /* No error checking at present */

    MPL_UNREFERENCED_ARG(comm);
    MPL_UNREFERENCED_ARG(keyval);
    MPL_UNREFERENCED_ARG(extra_state);

    return attr_dup_fn(attr_in, attr_out, flag);
}

#ifdef BUILD_MPI_ABI
int MPIR_Comm_dup_fn(ABI_Comm comm, int keyval, void *extra_state,
                     void *attr_in, void *attr_out, int *flag)
{
    return attr_dup_fn(attr_in, attr_out, flag);
}

int MPIR_Type_dup_fn(ABI_Datatype datatype, int keyval, void *extra_state,
                     void *attr_in, void *attr_out, int *flag)
{
    return attr_dup_fn(attr_in, attr_out, flag);
}

int MPIR_Win_dup_fn(ABI_Win win, int keyval, void *extra_state,
                    void *attr_in, void *attr_out, int *flag)
{
    return attr_dup_fn(attr_in, attr_out, flag);
}
#endif
