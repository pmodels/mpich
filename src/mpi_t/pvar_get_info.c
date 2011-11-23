/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_pvar_get_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_pvar_get_info = PMPIX_T_pvar_get_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_pvar_get_info  MPIX_T_pvar_get_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_pvar_get_info as PMPIX_T_pvar_get_info
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_pvar_get_info
#define MPIX_T_pvar_get_info PMPIX_T_pvar_get_info

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_get_info_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_pvar_get_info_impl(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class, MPI_Datatype *datatype, MPIX_T_enum *enumtype, char *desc, int *desc_len, int *binding, int *readonly, int *continuous, int *atomic)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_T_pvar_info *info = NULL;

    MPIU_Assert(pvar_index >= 0);

    mpi_errno = MPIR_T_get_pvar_info_by_idx(pvar_index, &info);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Tool_strncpy(name, info->name, name_len);
    MPIU_Tool_strncpy(desc, info->desc, desc_len);

    *verbosity  = info->verbosity;
    *var_class  = info->varclass;
    *datatype   = info->dtype;
    *enumtype   = info->etype;
    *binding    = info->binding;
    *readonly   = info->readonly;
    *continuous = info->continuous;
    *atomic     = info->atomic;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_T_pvar_get_info
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_pvar_get_info - XXX description here

Input/Output Parameters:
+ name_len - length of the string and/or buffer for name (integer)
- desc_len - length of the string and/or buffer for desc (integer)

Input Parameters:
. pvar_index - index of the performance variable to be queried between 0 and num_pvar-1 (integer)

Output Parameters:
+ name - buffer to return the string containing the name of the performance variable (string)
. verbosity - verbosity level of this variable (integer)
. var_class - class of performance variable (integer)
. datatype - MPI type of the information stored in the performance variable (handle)
. enumtype - optional descriptor for enumeration information (handle)
. desc - buffer to return the string containing a description of the performance variable (string)
. binding - type of MPI object to which this variable must be bound (integer)
. readonly - flag indicating whether the variable can be written/reset (integer)
. continuous - flag indicating whether the variable can be started and stopped or is continuously active (integer)
- atomic - flag indicating whether the variable can be atomically read and reset (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_pvar_get_info(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class, MPI_Datatype *datatype, MPIX_T_enum *enumtype, char *desc, int *desc_len, int *binding, int *readonly, int *continuous, int *atomic)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_PVAR_GET_INFO);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_PVAR_GET_INFO);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_ARGNULL(name_len, "name_len", mpi_errno);
            MPIR_ERRTEST_ARGNULL(verbosity, "verbosity", mpi_errno);
            MPIR_ERRTEST_ARGNULL(var_class, "var_class", mpi_errno);
            MPIR_ERRTEST_ARGNULL(desc_len, "desc_len", mpi_errno);
            MPIR_ERRTEST_ARGNULL(binding, "binding", mpi_errno);
            MPIR_ERRTEST_ARGNULL(readonly, "readonly", mpi_errno);
            MPIR_ERRTEST_ARGNULL(continuous, "continuous", mpi_errno);
            MPIR_ERRTEST_ARGNULL(atomic, "atomic", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_pvar_get_info_impl(pvar_index, name, name_len, verbosity, var_class, datatype, enumtype, desc, desc_len, binding, readonly, continuous, atomic);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_PVAR_GET_INFO);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_pvar_get_info", "**mpix_t_pvar_get_info %d %p %p %p %p %p %p %p %p %p %p %p %p", pvar_index, name, name_len, verbosity, var_class, datatype, enumtype, desc, desc_len, binding, readonly, continuous, atomic);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
