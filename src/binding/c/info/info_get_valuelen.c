/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get_valuelen */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get_valuelen = PMPI_Info_get_valuelen
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get_valuelen  MPI_Info_get_valuelen
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get_valuelen as PMPI_Info_get_valuelen
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag)
     __attribute__ ((weak, alias("PMPI_Info_get_valuelen")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_get_valuelen
#define MPI_Info_get_valuelen PMPI_Info_get_valuelen

#endif

/*@
   MPI_Info_get_valuelen - Retrieves the length of the value associated with a key

Input Parameters:
+ info - info object (handle)
- key - key (string)

Output Parameters:
+ valuelen - length of value arg (integer)
- flag - true if key defined, false if not (boolean)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_INFO
@*/

int MPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *info_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INFO_GET_VALUELEN);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INFO_GET_VALUELEN);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Info_get_ptr(info, info_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Info_valid_ptr(info_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERR_CHKANDJUMP((!key), mpi_errno, MPI_ERR_INFO_KEY, "**infokeynull");
            int keylen = (int) strlen(key);
            MPIR_ERR_CHKANDJUMP((keylen > MPI_MAX_INFO_KEY), mpi_errno, MPI_ERR_INFO_KEY, "**infokeylong");
            MPIR_ERR_CHKANDJUMP((keylen == 0), mpi_errno, MPI_ERR_INFO_KEY, "**infokeyempty");
            MPIR_ERRTEST_ARGNULL(valuelen, "valuelen", mpi_errno);
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_Info_get_valuelen_impl(info_ptr, key, valuelen, flag);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INFO_GET_VALUELEN);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_info_get_valuelen", "**mpi_info_get_valuelen %I %s %p %p",
                                     info, key, valuelen, flag);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
