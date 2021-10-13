/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"
/* mpir_err.h for the error code creation */
#include "mpir_err.h"
#include <stdio.h>

/* -- Begin Profiling Symbol Block for routine MPI_Status_f2c */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Status_f2c = PMPI_Status_f2c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Status_f2c  MPI_Status_f2c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Status_f2c as PMPI_Status_f2c
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Status_f2c(const MPI_Fint * f_status, MPI_Status * c_status)
    __attribute__ ((weak, alias("PMPI_Status_f2c")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Status_f2c
#define MPI_Status_f2c PMPI_Status_f2c
#endif


int MPI_Status_f2c(const MPI_Fint * f_status, MPI_Status * c_status)
{
    int mpi_errno = MPI_SUCCESS;

    if (f_status == MPI_F_STATUS_IGNORE) {
        /* The call is erroneous (see 4.12.5 in MPI-2) */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPI_Status_f2c", __LINE__, MPI_ERR_OTHER,
                                         "**notfstatignore", 0);
        return MPIR_Err_return_comm(0, "MPI_Status_f2c", mpi_errno);
    }
#ifdef HAVE_FINT_IS_INT
    *c_status = *(MPI_Status *) f_status;
#else
    c_status->count_lo = (int) f_status[0];
    c_status->count_hi_and_cancelled = (int) f_status[1];
    c_status->MPI_SOURCE = (int) f_status[2];
    c_status->MPI_TAG = (int) f_status[3];
    c_status->MPI_ERROR = (int) f_status[4];
    /* no need to copy abi_slush_fund field */
#endif

    return MPI_SUCCESS;
}
