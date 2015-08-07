/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_enum_get_item */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_enum_get_item = PMPI_T_enum_get_item
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_enum_get_item  MPI_T_enum_get_item
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_enum_get_item as PMPI_T_enum_get_item
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_enum_get_item(MPI_T_enum enumtype, int indx, int *value, char *name, int *name_len) __attribute__((weak,alias("PMPI_T_enum_get_item")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_enum_get_item
#define MPI_T_enum_get_item PMPI_T_enum_get_item
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_enum_get_item
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_enum_get_item - Get the information about an item in an enumeration

Input/Output Parameters:
. name_len - length of the string and/or buffer for name (integer)

Input Parameters:
. enumtype - enumeration to be queried (handle)

Output Parameters:
+ index - number of the value to be queried in this enumeration (integer)
. value - variable value (integer)
- name - buffer to return the string containing the name of the enumeration item (string)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_INVALID_ITEM
@*/
int MPI_T_enum_get_item(MPI_T_enum enumtype, int index, int *value, char *name, int *name_len)
{
    int mpi_errno = MPI_SUCCESS;
    enum_item_t *item;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_ENUM_GET_ITEM);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_ENUM_GET_ITEM);

    /* Validate parameters */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_ENUM_HANDLE(enumtype, mpi_errno);
            MPIR_ERRTEST_ENUM_ITEM(enumtype, index, mpi_errno);
            MPIR_ERRTEST_ARGNULL(value, "value", mpi_errno);
            /* Do not do TEST_ARGNULL for name or name_len, since this is
             * permitted per MPI_T standard.
             */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    item = (enum_item_t *)utarray_eltptr(enumtype->items, index);
    *value = item->value;
    MPIR_T_strncpy(name, item->name, name_len);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_ENUM_GET_ITEM);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_enum_get_item", "**mpi_t_enum_get_item %p %d %p %p %p",
            enumtype, index, value, name, name_len);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
