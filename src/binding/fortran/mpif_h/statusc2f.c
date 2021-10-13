/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"
/* mpir_err.h for the error code creation */
#include "mpir_err.h"
#include <stdio.h>

/* -- Begin Profiling Symbol Block for routine MPI_Status_c2f */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Status_c2f = PMPI_Status_c2f
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Status_c2f MPI_Status_c2f
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Status_c2f as PMPI_Status_c2f
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Status_c2f(const MPI_Status * c_status, MPI_Fint * f_status)
    __attribute__ ((weak, alias("PMPI_Status_c2f")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Status_c2f
#define MPI_Status_c2f PMPI_Status_c2f
#endif


int MPI_Status_c2f(const MPI_Status * c_status, MPI_Fint * f_status)
{
    int mpi_errno = MPI_SUCCESS;
    if (c_status == MPI_STATUS_IGNORE || c_status == MPI_STATUSES_IGNORE) {
        /* The call is erroneous (see 4.12.5 in MPI-2) */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPI_Status_c2f", __LINE__, MPI_ERR_OTHER,
                                         "**notcstatignore", 0);
        return MPIR_Err_return_comm(0, "MPI_Status_c2f", mpi_errno);
    }
#ifdef HAVE_FINT_IS_INT
    *(MPI_Status *) f_status = *c_status;
#else
    f_status[0] = (MPI_Fint) c_status->count_lo;
    f_status[1] = (MPI_Fint) c_status->count_hi_and_cancelled;
    f_status[2] = (MPI_Fint) c_status->MPI_SOURCE;
    f_status[3] = (MPI_Fint) c_status->MPI_TAG;
    f_status[4] = (MPI_Fint) c_status->MPI_ERROR;
#endif

    return MPI_SUCCESS;
}
