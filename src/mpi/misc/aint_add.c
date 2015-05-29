/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Aint_add */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Aint_add = PMPI_Aint_add
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Aint_add  MPI_Aint_add
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Aint_add as PMPI_Aint_add
#elif defined(HAVE_WEAK_ATTRIBUTE)
MPI_Aint MPI_Aint_add(MPI_Aint base, MPI_Aint disp) __attribute__((weak,alias("PMPI_Aint_add")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Aint_add
#define MPI_Aint_add PMPI_Aint_add

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Aint_add

/*@
MPI_Aint_add - Returns the sum of base and disp

Input Parameters:
+ base - base address (integer)
- disp - displacement (integer)

Return value:
Sum of the base and disp argument

Notes:
MPI_Aint_Add produces a new MPI_Aint value that is equivalent to the sum of the
base and disp arguments, where base represents a base address returned by a call
to MPI_GET_ADDRESS and disp represents a signed integer displacement. The resulting
address is valid only at the process that generated base, and it must correspond
to a location in the same object referenced by base. The addition is performed in
a manner that results in the correct MPI_Aint representation of the output address,
as if the process that originally produced base had called:
    MPI_Get_address((char *) base + disp, &result)

.seealso: MPI_Aint_diff
@*/

MPI_Aint MPI_Aint_add(MPI_Aint base, MPI_Aint disp)
{
    MPI_Aint result;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_AINT_ADD);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_AINT_ADD);
    result = MPID_Aint_add(base, disp);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_AINT_ADD);

    return result;
}
