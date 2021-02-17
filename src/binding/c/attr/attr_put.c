/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Attr_put */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Attr_put = PMPI_Attr_put
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Attr_put  MPI_Attr_put
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Attr_put as PMPI_Attr_put
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val)
     __attribute__ ((weak, alias("PMPI_Attr_put")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Attr_put
#define MPI_Attr_put PMPI_Attr_put

#endif

/*@
   MPI_Attr_put - Stores attribute value associated with a key

Input Parameters:
+ comm - communicator to which attribute will be attached (handle)
. keyval - key value, as returned by MPI_KEYVAL_CREATE (integer)
- attribute_val - attribute value (None)

.N Deprecated
   The replacement for this routine is 'MPI_Comm_set_attr'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Attr_get, MPI_Keyval_create, MPI_Attr_delete, MPI_Comm_set_attr
@*/

int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ATTR_PUT);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ATTR_PUT);

    mpi_errno = PMPI_Comm_set_attr(comm, keyval, attribute_val);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ATTR_PUT);
    return mpi_errno;
}
