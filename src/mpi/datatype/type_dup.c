/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_dup = PMPI_Type_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_dup  MPI_Type_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_dup as PMPI_Type_dup
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype * newtype)
    __attribute__ ((weak, alias("PMPI_Type_dup")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_dup
#define MPI_Type_dup PMPI_Type_dup

#endif

int MPIR_Type_dup(MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp = 0, *old_dtp;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        /* create a new type and commit it. */
        mpi_errno = MPIR_Type_contiguous(1, oldtype, newtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* allocate new datatype object and handle */
        new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        if (!new_dtp) {
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             "MPIR_Type_dup", __LINE__, MPI_ERR_OTHER,
                                             "**nomem", 0);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        /* fill in datatype */
        MPIR_Object_set_ref(new_dtp, 1);
        /* new_dtp->handle is filled in by MPIR_Handle_obj_alloc() */
        new_dtp->is_contig = old_dtp->is_contig;
        new_dtp->size = old_dtp->size;
        new_dtp->extent = old_dtp->extent;
        new_dtp->ub = old_dtp->ub;
        new_dtp->lb = old_dtp->lb;
        new_dtp->true_ub = old_dtp->true_ub;
        new_dtp->true_lb = old_dtp->true_lb;
        new_dtp->alignsize = old_dtp->alignsize;
        new_dtp->is_committed = old_dtp->is_committed;

        new_dtp->attributes = NULL;     /* Attributes are copied in the
                                         * top-level MPI_Type_dup routine */
        new_dtp->name[0] = 0;   /* The Object name is not copied on
                                 * a dup */
        new_dtp->n_builtin_elements = old_dtp->n_builtin_elements;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type = old_dtp->basic_type;

        new_dtp->typerep.handle = NULL;
        *newtype = new_dtp->handle;

        mpi_errno = MPIR_Typerep_create_dup(oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);

        /* if old_dtp is commited, user will not call `MPI_Type_commit` on the new type,
         * but the device still need be notified (e.g. ucx need register the type) */
        if (old_dtp->is_committed) {
            MPID_Type_commit_hook(new_dtp);
        }
    }

    MPL_DBG_MSG_D(MPIR_DBG_DATATYPE, VERBOSE, "dup type %x created.", *newtype);

  fn_fail:
    return mpi_errno;
}

/*@
   MPI_Type_dup - Duplicate a datatype

Input Parameters:
. oldtype - datatype (handle)

Output Parameters:
. newtype - copy of type (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *datatype_ptr = NULL;
    MPIR_Datatype *new_dtp;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_DUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_DUP);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Datatype_get_ptr(oldtype, datatype_ptr);

    /* Convert MPI object handles to object pointers */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            /* If comm_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */
    MPIR_Assert(datatype_ptr != NULL);

    /* ... body of routine ...  */

    mpi_errno = MPIR_Type_dup(oldtype, &new_handle);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_DUP, 0,        /* ints */
                                           0,   /* aints */
                                           1,   /* types */
                                           NULL, NULL, &oldtype);

    mpi_errno = MPIR_Type_commit(&new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    /* Copy attributes, executing the attribute copy functions */
    /* This accesses the attribute dup function through the perprocess
     * structure to prevent type_dup from forcing the linking of the
     * attribute functions.  The actual function is (by default)
     * MPIR_Attr_dup_list
     */
    if (mpi_errno == MPI_SUCCESS && MPIR_Process.attr_dup) {
        new_dtp->attributes = 0;
        mpi_errno = MPIR_Process.attr_dup(oldtype, datatype_ptr->attributes, &new_dtp->attributes);
        if (mpi_errno) {
            MPIR_Datatype_ptr_release(new_dtp);
            goto fn_fail;
        }
    }

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_DUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *newtype = MPI_DATATYPE_NULL;
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_dup", "**mpi_type_dup %D %p", oldtype, newtype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
