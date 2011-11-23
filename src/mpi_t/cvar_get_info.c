/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_cvar_get_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_cvar_get_info = PMPIX_T_cvar_get_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_cvar_get_info  MPIX_T_cvar_get_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_cvar_get_info as PMPIX_T_cvar_get_info
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_cvar_get_info
#define MPIX_T_cvar_get_info PMPIX_T_cvar_get_info

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_cvar_get_info_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_cvar_get_info_impl(int cvar_index, char *name, int *name_len, int *verbosity, MPI_Datatype *datatype, MPIX_T_enum *enumtype, char *desc, int *desc_len, int *binding, int *scope)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_Param_t *p = NULL;

    /* TODO convert to real error handling */
    MPIU_Assert(cvar_index >= 0);
    MPIU_Assert(cvar_index < MPIR_PARAM_NUM_PARAMS);

    p = &MPIR_Param_params[cvar_index];
    MPIU_Tool_strncpy(name, p->name, name_len);
    MPIU_Tool_strncpy(desc, p->description, desc_len);

    /* we do not currently have verbosity levels, so the draft standard says to
     * return USER_BASIC */
    *verbosity = MPIX_T_VERBOSITY_USER_BASIC;

    switch (p->default_val.type) {
        case MPIR_PARAM_TYPE_INT:
            *datatype = MPI_INT;
            break;
        case MPIR_PARAM_TYPE_DOUBLE:
            *datatype = MPI_DOUBLE;
            break;
        case MPIR_PARAM_TYPE_BOOLEAN:
            *datatype = MPI_INT;
            break;
        case MPIR_PARAM_TYPE_STRING:
            *datatype = MPI_CHAR;
            break;
        case MPIR_PARAM_TYPE_RANGE:
            /* type is int, but count should be 2 */
            *datatype = MPI_INT;
            break;
        default:
            /* FIXME the error handling code may not have been setup yet */
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**intern", "**intern %s", "unexpected parameter type");
            break;
    }

    *binding = MPIX_T_BIND_NO_OBJECT;

    /* FIXME a bit of a fib, may not actually be true for any given cvar */
    *scope = MPIX_T_SCOPE_LOCAL;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_T_cvar_get_info
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_cvar_get_info - XXX description here

Input/Output Parameters:
+ name_len - length of the string and/or buffer for name (integer)
- desc_len - length of the string and/or buffer for desc (integer)

Input Parameters:
. cvar_index - index of the control variable to be queried, value between 0 and num_cvar-1 (integer)

Output Parameters:
+ name - buffer to return the string containing the name of the control variable (string)
. verbosity - verbosity level of this variable (integer)
. datatype - MPI datatype of the information stored in the control variable (handle)
. enumtype - optional descriptor for enumeration information (handle)
. desc - buffer to return the string containing a description of the control variable (string)
. binding - type of MPI object this variable is associated with
- scope - scope of when changes to this variable are possible (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity, MPI_Datatype *datatype, MPIX_T_enum *enumtype, char *desc, int *desc_len, int *binding, int *scope)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_CVAR_GET_INFO);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_CVAR_GET_INFO);

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
            MPIR_ERRTEST_ARGNULL(desc_len, "desc_len", mpi_errno);
            MPIR_ERRTEST_ARGNULL(binding, "binding", mpi_errno);
            MPIR_ERRTEST_ARGNULL(scope, "scope", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_cvar_get_info_impl(cvar_index, name, name_len, verbosity, datatype, enumtype, desc, desc_len, binding, scope);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_CVAR_GET_INFO);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_cvar_get_info", "**mpix_t_cvar_get_info %d %p %p %p %p %p %p %p %p %p", cvar_index, name, name_len, verbosity, datatype, enumtype, desc, desc_len, binding, scope);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
