/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_enum_get_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_enum_get_info = PMPI_T_enum_get_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_enum_get_info  MPI_T_enum_get_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_enum_get_info as PMPI_T_enum_get_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len)
    __attribute__ ((weak, alias("PMPI_T_enum_get_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_enum_get_info
#define MPI_T_enum_get_info PMPI_T_enum_get_info
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_enum_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_enum_get_info - Get the information about an enumeration

Input/Output Parameters:
. name_len - length of the string and/or buffer for name (integer)

Input Parameters:
. enumtype - enumeration to be queried (handle)

Output Parameters:
+ num - number of discrete values represented by this enumeration (integer)
- name - buffer to return the string containing the name of the enumeration (string)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_HANDLE
@*/
int MPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_ENUM_GET_INFO);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_ENUM_GET_INFO);

    /* Validate parameters */
    MPIT_ERRTEST_ENUM_HANDLE(enumtype);
    MPIT_ERRTEST_ARGNULL(num);

    /* ... body of routine ...  */

    *num = utarray_len(enumtype->items);
    MPIR_T_strncpy(name, enumtype->name, name_len);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_ENUM_GET_INFO);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
