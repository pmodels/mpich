/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

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

#undef FUNCNAME
#define FUNCNAME MPII_Type_get_attr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val,
                       int *flag, MPIR_Attr_type outAttrType)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *type_ptr = NULL;
    MPIR_Attribute *p;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_TYPE_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_TYPE_GET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_KEYVAL(type_keyval, MPIR_DATATYPE, "datatype", mpi_errno);
#ifdef NEEDS_POINTER_ALIGNMENT_ADJUST
            /* A common user error is to pass the address of a 4-byte
             * int when the address of a pointer (or an address-sized int)
             * should have been used.  We can test for this specific
             * case.  Note that this code assumes sizeof(intptr_t) is
             * a power of 2. */
            if ((intptr_t) attribute_val & (sizeof(intptr_t) - 1)) {
                MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**attrnotptr");
            }
#endif
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

    *flag = 0;
    p = type_ptr->attributes;
    while (p) {
        if (p->keyval->handle == type_keyval) {
            *flag = 1;
            if (outAttrType == MPIR_ATTR_PTR) {
                if (p->attrType == MPIR_ATTR_INT) {
                    /* This is the tricky case: if the system is
                     * bigendian, and we have to return a pointer to
                     * an int, then we may need to point to the
                     * correct location in the word. */
#if defined(WORDS_LITTLEENDIAN) || (SIZEOF_VOID_P == SIZEOF_INT)
                    *(void **) attribute_val = &(p->value);
#else
                    int *p_loc = (int *) &(p->value);
#if SIZEOF_VOID_P == 2 * SIZEOF_INT
                    p_loc++;
#else
#error Expected sizeof(void*) to be either sizeof(int) or 2*sizeof(int)
#endif
                    *(void **) attribute_val = p_loc;
#endif
                } else if (p->attrType == MPIR_ATTR_AINT) {
                    *(void **) attribute_val = &(p->value);
                } else {
                    *(void **) attribute_val = (void *) (intptr_t) (p->value);
                }
            } else
                *(void **) attribute_val = (void *) (intptr_t) (p->value);

            break;
        }
        p = p->next;
    }

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_TYPE_GET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpir_type_get_attr", "**mpir_type_get_attr %D %d %p %p",
                                 datatype, type_keyval, attribute_val, flag);
    }
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
#endif
    /* --END ERROR HANDLING-- */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_attr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

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
int MPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_GET_ATTR);

    /* ... body of routine ...  */
    mpi_errno = MPII_Type_get_attr(datatype, type_keyval, attribute_val, flag, MPIR_ATTR_PTR);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_GET_ATTR);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_get_attr", "**mpi_type_get_attr %D %d %p %p", datatype,
                                 type_keyval, attribute_val, flag);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
