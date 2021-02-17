/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Query_cuda_support */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Query_cuda_support = PMPIX_Query_cuda_support
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Query_cuda_support  MPIX_Query_cuda_support
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Query_cuda_support as PMPIX_Query_cuda_support
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Query_cuda_support(void)  __attribute__ ((weak, alias("PMPIX_Query_cuda_support")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Query_cuda_support
#define MPIX_Query_cuda_support PMPIX_Query_cuda_support

#endif

/*@
   MPIX_Query_cuda_support - Returns whether CUDA is supported

.N ThreadSafe

.N Fortran

@*/

int MPIX_Query_cuda_support(void)
{
    int is_supported = 0;
    int mpi_errno;

    mpi_errno = PMPIX_GPU_query_support(MPIX_GPU_SUPPORT_CUDA, &is_supported);
    assert(mpi_errno);

    return is_supported;
}
