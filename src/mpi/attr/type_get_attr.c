/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_attr = PMPI_Type_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_attr  MPI_Type_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_attr as PMPI_Type_get_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag)
    __attribute__ ((weak, alias("PMPI_Type_get_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_attr
#define MPI_Type_get_attr PMPI_Type_get_attr

int MPII_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag,
                       MPIR_Attr_type attr_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *type_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPII_TYPE_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPII_TYPE_GET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_KEYVAL(type_keyval, MPIR_DATATYPE, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Datatype_get_ptr(datatype, type_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate datatype pointer */
            MPIR_Datatype_valid_ptr(type_ptr, mpi_errno);
            /* If type_ptr is not valid, it will be reset to null */
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Type_get_attr_impl(type_ptr, type_keyval, attribute_val, flag, attr_type);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPII_TYPE_GET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_get_attr", "**mpi_type_get_attr %D %d %p %p",
                                 datatype, type_keyval, attribute_val, flag);
    }
#endif
    /* --END ERROR HANDLING-- */
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
}
#endif


int MPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag)
{
    QMPI_Context context;
    QMPI_Type_get_attr_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_Type_get_attr(context, 0, datatype, type_keyval, attribute_val, flag);

    fn_ptr = (QMPI_Type_get_attr_t *) MPIR_QMPI_first_fn_ptrs[MPI_TYPE_GET_ATTR_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_TYPE_GET_ATTR_T], datatype, type_keyval,
                      attribute_val, flag);
}

/*@
   MPI_Type_get_attr - Retrieves attribute value by key

Input Parameters:
+ datatype - datatype to which the attribute is attached (handle)
- type_keyval - key value (integer)

Output Parameters:
+ attribute_val - attribute value, unless flag = false
- flag - false if no attribute is associated with the key (logical)

   Notes:
    Attributes must be extracted from the same language as they were inserted
    in with 'MPI_Type_set_attr'.  The notes for C and Fortran below explain
    why.

Notes for C:
    Even though the 'attr_value' argument is declared as 'void *', it is
    really the address of a void pointer.  See the rationale in the
    standard for more details.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_KEYVAL
.N MPI_ERR_ARG
@*/
int QMPI_Type_get_attr(QMPI_Context context, int tool_id, MPI_Datatype datatype, int type_keyval,
                       void *attribute_val, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_GET_ATTR);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_GET_ATTR);

    mpi_errno = MPII_Type_get_attr(datatype, type_keyval, attribute_val, flag, MPIR_ATTR_PTR);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_GET_ATTR);
    return mpi_errno;
}
