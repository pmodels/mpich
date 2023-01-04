/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_THREADCOMM

int MPIR_Threadcomm_set_errhandler_impl(MPIR_Comm * comm, MPIR_Errhandler * errhandler_ptr)
{
    return MPIR_Comm_set_errhandler_impl(comm, errhandler_ptr);
}

#endif /* ENABLE_THREADCOMM */
