/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_cvar_read */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_cvar_read = PMPIX_T_cvar_read
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_cvar_read  MPIX_T_cvar_read
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_cvar_read as PMPIX_T_cvar_read
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_cvar_read
#define MPIX_T_cvar_read PMPIX_T_cvar_read

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_cvar_read_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_cvar_read_impl(MPIX_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_Param_t *p = handle->p;

    switch (p->default_val.type) {
        case MPIR_PARAM_TYPE_INT:
            {
                int *i_buf = buf;
                *i_buf = *(int *)p->val_p;
            }
            break;
        case MPIR_PARAM_TYPE_DOUBLE:
            {
                double *d_buf = buf;
                *d_buf = *(double *)p->val_p;
            }
            break;
        case MPIR_PARAM_TYPE_BOOLEAN:
            {
                int *i_buf = buf;
                *i_buf = *(int *)p->val_p;
            }
            break;
        case MPIR_PARAM_TYPE_STRING:
            if (*(char **)p->val_p == NULL) {
                char *c_buf = buf;
                c_buf[0] = '\0';
            }
            else {
                MPIU_Strncpy(buf, *(char **)p->val_p, MPIR_PARAM_MAX_STRLEN);
            }
            break;
        case MPIR_PARAM_TYPE_RANGE:
            MPIU_Memcpy(buf, p->val_p, 2*sizeof(int));
            break;
        default:
            /* FIXME the error handling code may not have been setup yet */
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**intern", "**intern %s", "unexpected parameter type");
            break;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_T_cvar_read
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_cvar_read - XXX description here

Input Parameters:
. handle - handle to the control variable to be read (handle)

Output Parameters:
. buf - initial address of storage location for variable value

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_cvar_read(MPIX_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_CVAR_READ);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_CVAR_READ);

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
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_cvar_read_impl(handle, buf);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_CVAR_READ);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_cvar_read", "**mpix_t_cvar_read %p %p", handle, buf);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
