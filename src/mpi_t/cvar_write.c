/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_cvar_write */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_cvar_write = PMPIX_T_cvar_write
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_cvar_write  MPIX_T_cvar_write
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_cvar_write as PMPIX_T_cvar_write
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_cvar_write
#define MPIX_T_cvar_write PMPIX_T_cvar_write

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_cvar_write_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_cvar_write_impl(MPIX_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_Param_t *p = handle->p;

    switch (p->default_val.type) {
        case MPIR_PARAM_TYPE_INT:
            {
                int *ip_buf = (int *)p->val_p;
                *ip_buf = *(int *)buf;
            }
            break;
        case MPIR_PARAM_TYPE_DOUBLE:
            {
                double *dp_buf = (double *)p->val_p;
                *dp_buf = *(double *)buf;
            }
            break;
        case MPIR_PARAM_TYPE_BOOLEAN:
            {
                int *ip_buf = (int *)p->val_p;
                *ip_buf = *(int *)buf;
            }
            break;
        case MPIR_PARAM_TYPE_STRING:
            {
                char **lval_ptr = (char **)p->val_p;
                if (*lval_ptr != NULL) {
                    /* FIXME these strings are not all (in fact, almost never)
                     * allocated with malloc/strdup right now */
                    MPIU_Free(*lval_ptr);
                    *lval_ptr = NULL;
                }
                *lval_ptr = MPIU_Strdup(buf);
            }
            break;
        case MPIR_PARAM_TYPE_RANGE:
            MPIU_Memcpy(p->val_p, buf, 2*sizeof(int));
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
#define FUNCNAME MPIX_T_cvar_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_cvar_write - XXX description here

Input Parameters:
+ handle - handle of the control variable to be written (handle)
- buf - initial address of storage location for variable value (choice)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_cvar_write(MPIX_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_CVAR_WRITE);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_CVAR_WRITE);

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

    mpi_errno = MPIR_T_cvar_write_impl(handle, buf);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_CVAR_WRITE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_cvar_write", "**mpix_t_cvar_write %p %p", handle, buf);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
