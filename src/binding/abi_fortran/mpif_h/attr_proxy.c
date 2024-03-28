/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"

static int MPII_copy_attr_f90_proxy(MPI_Comm_copy_attr_function * user_function, int handle,
                                    int keyval, void *extra_state, MPIR_Attr_type value_type,
                                    void *value, void **new_value, int *flag)
{
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = (MPI_Fint) handle;
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint fvalue = (MPI_Aint) value;
    MPI_Aint *fextra = (MPI_Aint *) extra_state;
    MPI_Aint fnew = 0;
    MPI_Fint fflag = 0;

    ((F90_CopyFunction *) (void *) user_function) (&fhandle, &fkeyval, fextra, &fvalue, &fnew,
                                                   &fflag, &ierr);

    *flag = MPII_FROM_FLOG(fflag);
    *new_value = (void *) fnew;
    return (int) ierr;
}

static int MPII_delete_attr_f90_proxy(MPI_Comm_delete_attr_function * user_function, int handle,
                                      int keyval, MPIR_Attr_type value_type, void *value,
                                      void *extra_state)
{
    MPI_Fint ierr = 0;
    MPI_Fint fhandle = (MPI_Fint) handle;
    MPI_Fint fkeyval = (MPI_Fint) keyval;
    MPI_Aint fvalue = (MPI_Aint) value;
    MPI_Aint *fextra = (MPI_Aint *) extra_state;

    ((F90_DeleteFunction *) (void *) user_function) (&fhandle, &fkeyval, &fvalue, fextra, &ierr);
    return (int) ierr;
}

void MPII_Keyval_set_f90_proxy(int keyval)
{
    MPII_Keyval_set_proxy(keyval, MPII_copy_attr_f90_proxy, MPII_delete_attr_f90_proxy);
}
