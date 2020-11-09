/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Attr_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Attr_get = PMPI_Attr_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Attr_get  MPI_Attr_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Attr_get as PMPI_Attr_get
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag)
    __attribute__ ((weak, alias("PMPI_Attr_get")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Attr_get
#define MPI_Attr_get PMPI_Attr_get

#endif

/*@

MPI_Attr_get - Retrieves attribute value by key

Input Parameters:
+ comm - communicator to which attribute is attached (handle)
- keyval - key value (integer)

Output Parameters:
+ attribute_val - attribute value, unless 'flag' = false
- flag -  true if an attribute value was extracted;  false if no attribute is
  associated with the key

Notes:
    Attributes must be extracted from the same language as they were inserted
    in with 'MPI_ATTR_PUT'.  The notes for C and Fortran below explain why.

Notes for C:
    Even though the 'attribute_val' argument is declared as 'void *', it is
    really the address of a void pointer (i.e., a 'void **').  Using
    a 'void *', however, is more in keeping with C idiom and allows the
    pointer to be passed without additional casts.


.N ThreadSafe

.N Deprecated
   The replacement for this routine is 'MPI_Comm_get_attr'.

.N Fortran

    The 'attribute_val' in Fortran is a pointer to a Fortran integer, not
    a pointer to a 'void *'.

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_KEYVAL

.seealso MPI_Attr_put, MPI_Keyval_create, MPI_Attr_delete, MPI_Comm_get_attr
@*/
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ATTR_GET);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ATTR_GET);

    mpi_errno = PMPI_Comm_get_attr(comm, keyval, attribute_val, flag);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ATTR_GET);
    return mpi_errno;
}
