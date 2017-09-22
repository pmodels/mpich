/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_contigous */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_contiguous = PMPI_Type_contiguous
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_contiguous  MPI_Type_contiguous
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_contiguous as PMPI_Type_contiguous
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype) __attribute__((weak,alias("PMPI_Type_contiguous")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_contiguous
#define MPI_Type_contiguous PMPI_Type_contiguous

/*@
  MPIR_Type_contiguous - create a contiguous datatype

Input Parameters:
+ count - number of elements in the contiguous block
- oldtype - type (using handle) of datatype on which vector is based

Output Parameters:
. newtype - handle of new contiguous datatype

  Return Value:
  MPI_SUCCESS on success, MPI error code on failure.
@*/
int MPIR_Type_contiguous(int count,
                         MPI_Datatype oldtype,
                         MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin;
    MPI_Aint el_sz;
    MPI_Datatype el_type;
    MPIR_Datatype *new_dtp;

    if (count == 0) return MPII_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_contiguous",
                                         __LINE__, MPI_ERR_OTHER,
                                         "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

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

    if (is_builtin)
    {
        el_sz   = MPIR_Datatype_get_basic_size(oldtype);
        el_type = oldtype;

        new_dtp->size          = count * el_sz;
        new_dtp->has_sticky_ub = 0;
        new_dtp->has_sticky_lb = 0;
        new_dtp->true_lb       = 0;
        new_dtp->lb            = 0;
        new_dtp->true_ub       = count * el_sz;
        new_dtp->ub            = new_dtp->true_ub;
        new_dtp->extent        = new_dtp->ub - new_dtp->lb;

        new_dtp->alignsize     = el_sz;
        new_dtp->n_builtin_elements    = count;
        new_dtp->builtin_element_size  = el_sz;
        new_dtp->basic_type        = el_type;
        new_dtp->is_contig     = 1;
        new_dtp->max_contig_blocks = 1;

    }
    else
    {
        /* user-defined base type (oldtype) */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);
        el_sz   = old_dtp->builtin_element_size;
        el_type = old_dtp->basic_type;

        new_dtp->size           = count * old_dtp->size;
        new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;
        new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;

        MPII_DATATYPE_CONTIG_LB_UB((MPI_Aint) count,
                                   old_dtp->lb,
                                   old_dtp->ub,
                                   old_dtp->extent,
                                   new_dtp->lb,
                                   new_dtp->ub);

        /* easiest to calc true lb/ub relative to lb/ub; doesn't matter
         * if there are sticky lb/ubs or not when doing this.
         */
        new_dtp->true_lb = new_dtp->lb + (old_dtp->true_lb - old_dtp->lb);
        new_dtp->true_ub = new_dtp->ub + (old_dtp->true_ub - old_dtp->ub);
        new_dtp->extent  = new_dtp->ub - new_dtp->lb;

        new_dtp->alignsize    = old_dtp->alignsize;
        new_dtp->n_builtin_elements   = count * old_dtp->n_builtin_elements;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type       = el_type;

        MPIR_Datatype_is_contig(oldtype, &new_dtp->is_contig);
        if(new_dtp->is_contig)
            new_dtp->max_contig_blocks = 1;
        else
            new_dtp->max_contig_blocks = count * old_dtp->max_contig_blocks;
    }

    *newtype = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE,VERBOSE,"contig type %x created.",
                   new_dtp->handle);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Type_contiguous_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_contiguous_impl(int count,
                              MPI_Datatype oldtype,
                              MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;
    MPI_Datatype new_handle;
    
    mpi_errno = MPIR_Type_contiguous(count,
				     oldtype,
				     &new_handle);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_CONTIGUOUS,
				           1, /* ints (count) */
				           0,
				           1,
				           &count,
				           NULL,
				           &oldtype);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

 fn_exit:
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}
#undef FUNCNAME
#define FUNCNAME MPIR_Type_contiguous_x_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_contiguous_x_impl(MPI_Count count,
                              MPI_Datatype oldtype,
			      MPI_Datatype *newtype)
{
    /* to make 'count' fit MPI-3 type processing routines (which take integer
     * counts), we construct a type consisting of N INT_MAX chunks followed by
     * a remainder.  e.g for a count of 4000000000 bytes you would end up with
     * one 2147483647-byte chunk followed immediately by a 1852516353-byte
     * chunk */
    MPI_Datatype chunks, remainder;
    MPI_Aint lb, extent, disps[2];
    int blocklens[2];
    MPI_Datatype types[2];
    int mpi_errno;

    /* truly stupendously large counts will overflow an integer with this math,
     * but that is a problem for a few decades from now.  Sorry, few decades
     * from now! */
    MPIR_Assert(count/INT_MAX == (int)(count/INT_MAX));
    int c = (int)(count/INT_MAX); /* OK to cast until 'count' is 256 bits */
    int r = count%INT_MAX;

    mpi_errno = MPIR_Type_vector_impl(c, INT_MAX, INT_MAX, oldtype, &chunks);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    mpi_errno = MPIR_Type_contiguous_impl(r, oldtype, &remainder);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIR_Type_get_extent_impl(oldtype, &lb, &extent);

    blocklens[0] = 1;      blocklens[1] = 1;
    disps[0]     = 0;      disps[1]     = c*extent*INT_MAX;
    types[0]     = chunks; types[1]     = remainder;

    mpi_errno = MPIR_Type_create_struct_impl(2, blocklens, disps, types, newtype);

    MPIR_Type_free_impl(&chunks);
    MPIR_Type_free_impl(&remainder);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
#endif


#undef FUNCNAME
#define FUNCNAME MPI_Type_contiguous
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Type_contiguous - Creates a contiguous datatype

Input Parameters:
+ count - replication count (nonnegative integer) 
- oldtype - old datatype (handle) 

Output Parameters:
. newtype - new datatype (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_EXHAUSTED
@*/
int MPI_Type_contiguous(int count,
			MPI_Datatype oldtype,
			MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_CONTIGUOUS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_CONTIGUOUS);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
	    
            if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(oldtype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    }
	    MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Type_contiguous_impl(count, oldtype, newtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_CONTIGUOUS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
    mpi_errno = MPIR_Err_create_code(
	mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_contiguous",
	"**mpi_type_contiguous %d %D %p", count, oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
