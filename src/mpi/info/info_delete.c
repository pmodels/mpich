/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_info.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_delete */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_delete = PMPI_Info_delete
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_delete  MPI_Info_delete
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_delete as PMPI_Info_delete
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Info_delete(MPI_Info info, const char *key)
    __attribute__ ((weak, alias("PMPI_Info_delete")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_delete
#define MPI_Info_delete PMPI_Info_delete
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_delete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
  MPI_Info_delete - Deletes a (key,value) pair from info

Input Parameters:
+ info - info object (handle)
- key - key (string)

.N NotThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Info_delete(MPI_Info info, const char *key)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *info_ptr = 0, *prev_ptr, *curr_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INFO_DELETE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INFO_DELETE);


    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            int keylen;

            /* Validate info_ptr */
            MPIR_Info_valid_ptr(info_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;

            /* Check key */
            MPIR_ERR_CHKANDJUMP((!key), mpi_errno, MPI_ERR_INFO_KEY, "**infokeynull");
            keylen = (int) strlen(key);
            MPIR_ERR_CHKANDJUMP((keylen > MPI_MAX_INFO_KEY), mpi_errno, MPI_ERR_INFO_KEY,
                                "**infokeylong");
            MPIR_ERR_CHKANDJUMP((keylen == 0), mpi_errno, MPI_ERR_INFO_KEY, "**infokeyempty");
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    prev_ptr = info_ptr;
    curr_ptr = info_ptr->next;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            MPL_free(curr_ptr->key);
            MPL_free(curr_ptr->value);
            prev_ptr->next = curr_ptr->next;
            MPIR_Handle_obj_free(&MPIR_Info_mem, curr_ptr);
            break;
        }
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    /* If curr_ptr is not defined, we never found the key */
    MPIR_ERR_CHKANDJUMP1((!curr_ptr), mpi_errno, MPI_ERR_INFO_NOKEY, "**infonokey",
                         "**infonokey %s", key);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INFO_DELETE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_info_delete", "**mpi_info_delete %I %s", info, key);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
