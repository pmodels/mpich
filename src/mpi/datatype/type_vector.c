/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_vector */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_vector = PMPI_Type_vector
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_vector  MPI_Type_vector
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_vector as PMPI_Type_vector
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype,
                    MPI_Datatype *newtype) __attribute__((weak,alias("PMPI_Type_vector")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_vector
#define MPI_Type_vector PMPI_Type_vector

/*@
  MPIR_Type_vector - create a vector datatype

Input Parameters:
+ count - number of blocks in vector
. blocklength - number of elements in each block
. stride - distance from beginning of one block to the next (see next
  parameter for units)
. strideinbytes - if nonzero, then stride is in bytes, otherwise stride
  is in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which vector is based

Output Parameters:
. newtype - handle of new vector datatype

  Return Value:
  0 on success, MPI error code on failure.
@*/
int MPIR_Type_vector(int count,
		     int blocklength,
		     MPI_Aint stride,
		     int strideinbytes,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin, old_is_contig;
    MPI_Aint el_sz, old_sz;
    MPI_Datatype el_type;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub, eff_stride;

    MPIR_Datatype *new_dtp;

    if (count == 0) return MPII_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    if (!new_dtp) {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPIR_Type_vector", __LINE__,
					 MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
	/* --END ERROR HANDLING-- */
    }

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin) {
	el_sz   = (MPI_Aint) MPIR_Datatype_get_basic_size(oldtype);
	el_type = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = el_sz;
	old_true_ub   = el_sz;
	old_sz        = el_sz;
	old_extent    = el_sz;
	old_is_contig = 1;

	new_dtp->size           = (MPI_Aint) count *
	                          (MPI_Aint) blocklength * el_sz;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->has_sticky_ub  = 0;

	new_dtp->alignsize    = el_sz; /* ??? */
	new_dtp->n_builtin_elements   = count * blocklength;
	new_dtp->builtin_element_size = el_sz;
	new_dtp->basic_type       = el_type;

	new_dtp->max_contig_blocks = count;

	eff_stride = (strideinbytes) ? stride : (stride * el_sz);
    }
    else /* user-defined base type (oldtype) */ {
	MPIR_Datatype *old_dtp;

	MPIR_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->builtin_element_size;
	el_type = old_dtp->basic_type;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_sz        = old_dtp->size;
	old_extent    = old_dtp->extent;
	MPIR_Datatype_is_contig(oldtype, &old_is_contig);

	new_dtp->size           = count * blocklength * old_dtp->size;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_builtin_elements   = count * blocklength * old_dtp->n_builtin_elements;
	new_dtp->builtin_element_size = el_sz;
	new_dtp->basic_type       = el_type;

	new_dtp->max_contig_blocks = old_dtp->max_contig_blocks * count * blocklength;

	eff_stride = (strideinbytes) ? stride : (stride * old_dtp->extent);
    }

    MPII_DATATYPE_VECTOR_LB_UB((MPI_Aint) count,
			       eff_stride,
			       (MPI_Aint) blocklength,
			       old_lb,
			       old_ub,
			       old_extent,
			       new_dtp->lb,
			       new_dtp->ub);
    new_dtp->true_lb = new_dtp->lb + (old_true_lb - old_lb);
    new_dtp->true_ub = new_dtp->ub + (old_true_ub - old_ub);
    new_dtp->extent  = new_dtp->ub - new_dtp->lb;

    /* new type is only contig for N types if old one was, and
     * size and extent of new type are equivalent, and stride is
     * equal to blocklength * size of old type.
     */
    if ((MPI_Aint)(new_dtp->size) == new_dtp->extent &&
	eff_stride == (MPI_Aint) blocklength * old_sz &&
	old_is_contig)
    {
	new_dtp->is_contig = 1;
        new_dtp->max_contig_blocks = 1;
    }
    else {
	new_dtp->is_contig = 0;
    }

    *newtype = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE,VERBOSE,"vector type %x created.",
		   new_dtp->handle);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Type_vector_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_vector_impl(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int ints[3];

    mpi_errno = MPIR_Type_vector(count,
				 blocklength,
				 (MPI_Aint) stride,
				 0, /* stride not in bytes, in extents */
				 oldtype,
				 &new_handle);

    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    ints[0] = count;
    ints[1] = blocklength;
    ints[2] = stride;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp,
                                           MPI_COMBINER_VECTOR,
                                           3, /* ints (cnt, blklen, str) */
                                           0, /* aints */
                                           1, /* types */
                                           ints,
                                           NULL,
                                           &oldtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_vector
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Type_vector - Creates a vector (strided) datatype

Input Parameters:
+ count - number of blocks (nonnegative integer) 
. blocklength - number of elements in each block 
  (nonnegative integer)
. stride - number of elements between start of each block (integer)
- oldtype - old datatype (handle)

Output Parameters:
. newtype - new datatype (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Type_vector(int count,
		    int blocklength,
		    int stride, 
		    MPI_Datatype oldtype,
		    MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_VECTOR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_VECTOR);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_Datatype *old_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNEG(blocklength, "blocklen", mpi_errno);
	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
	    
	    if (oldtype != MPI_DATATYPE_NULL && HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
		MPIR_Datatype_get_ptr(oldtype, old_ptr);
		MPIR_Datatype_valid_ptr(old_ptr, mpi_errno);
                if (mpi_errno) goto fn_fail;
	    }
           MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Type_vector_impl(count, blocklength, stride, oldtype, newtype);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_vector",
	    "**mpi_type_vector %d %d %d %D %p", count, blocklength, stride, oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
