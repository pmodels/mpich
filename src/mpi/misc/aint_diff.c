/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Aint_diff */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Aint_diff = PMPI_Aint_diff
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Aint_diff  MPI_Aint_diff
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Aint_diff as PMPI_Aint_diff
#elif defined(HAVE_WEAK_ATTRIBUTE)
MPI_Aint MPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2) __attribute__((weak,alias("PMPI_Aint_diff")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Aint_diff
#define MPI_Aint_diff PMPI_Aint_diff

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Aint_diff

/*@
MPI_Aint_diff - Returns the difference between addr1 and addr2

Input Parameters:
+ addr1 - minuend address (integer)
- addr2 - subtrahend address (integer)

Return value:
Difference between addr1 and addr2

Notes:
MPI_Aint_diff produces a new MPI_Aint value that is equivalent to the difference
between addr1 and addr2 arguments, where addr1 and addr2 represent addresses
returned by calls to MPI_GET_ADDRESS. The resulting address is valid only at the
process that generated addr1 and addr2, and addr1 and addr2 must correspond to
locations in the same object in the same process. The difference is calculated
in a manner that results the signed difference from addr1 to addr2, as if the
process that originally produced the addresses had called
    (char *) addr1 - (char *) addr2
on the addresses initially passed to MPI_GET_ADDRESS.

.seealso: MPI_Aint_add
@*/

MPI_Aint MPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
    MPI_Aint result;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_AINT_DIFF);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_AINT_DIFF);
    result = MPID_Aint_diff(addr1, addr2);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_AINT_DIFF);

    return result;
}
