/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Unpack */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Unpack = PMPI_Unpack
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Unpack  MPI_Unpack
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Unpack as PMPI_Unpack
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount,
               MPI_Datatype datatype, MPI_Comm comm) __attribute__((weak,alias("PMPI_Unpack")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Unpack
#define MPI_Unpack PMPI_Unpack

#undef FUNCNAME
#define FUNCNAME MPIR_Unpack_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Unpack_impl(const void *inbuf, MPI_Aint insize, MPI_Aint *position,
                     void *outbuf, int outcount, MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;
    MPID_Segment *segp;
    int contig;
    MPI_Aint dt_true_lb;
    MPI_Aint data_sz;
   
    if (insize == 0)
	goto fn_exit;

    /* Handle contig case quickly */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
        contig     = TRUE;
        dt_true_lb = 0;
        data_sz    = outcount * MPID_Datatype_get_basic_size(datatype);
    } else {
        MPID_Datatype *dt_ptr;
        MPID_Datatype_get_ptr(datatype, dt_ptr);
	contig     = dt_ptr->is_contig;
        dt_true_lb = dt_ptr->true_lb;
        data_sz    = outcount * dt_ptr->size;
    }

    if (contig) {
        MPIU_Memcpy((char *) outbuf + dt_true_lb, (char *)inbuf + *position, data_sz);
        *position = (int)((MPI_Aint)*position + data_sz);
        goto fn_exit;
    }
    

    /* non-contig case */
    segp = MPID_Segment_alloc();
    MPIR_ERR_CHKANDJUMP1(segp == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    mpi_errno = MPID_Segment_init(outbuf, outcount, datatype, segp, 0);
    MPIU_Assert(mpi_errno == MPI_SUCCESS);

    /* NOTE: the use of buffer values and positions in MPI_Unpack and in
     * MPID_Segment_unpack are quite different.  See code or docs or something.
     */
    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    /* Ensure that pointer increment fits in a pointer */
    MPIU_Ensure_Aint_fits_in_pointer((MPIU_VOID_PTR_CAST_TO_MPI_AINT inbuf) +
				     (MPI_Aint) *position);

    MPID_Segment_unpack(segp,
			first,
			&last,
			(void *) ((char *) inbuf + *position));

    /* Ensure that calculation fits into an int datatype. */
    MPIU_Ensure_Aint_fits_in_int((MPI_Aint)*position + last);

    *position = (int)((MPI_Aint)*position + last);

    MPID_Segment_free(segp);


 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Unpack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Unpack - Unpack a buffer according to a datatype into contiguous memory

Input Parameters:
+ inbuf - input buffer start (choice)
. insize - size of input buffer, in bytes (integer)
. outcount - number of items to be unpacked (integer)
. datatype - datatype of each output data item (handle)
- comm - communicator for packed message (handle)

Output Parameters:
. outbuf - output buffer start (choice)

Inout/Output Parameters:
. position - current position in bytes (integer)


.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_ARG

.seealso: MPI_Pack, MPI_Pack_size
@*/
int MPI_Unpack(const void *inbuf, int insize, int *position,
	       void *outbuf, int outcount, MPI_Datatype datatype,
	       MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint position_x;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_UNPACK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_UNPACK);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if (insize > 0) {
		MPIR_ERRTEST_ARGNULL(inbuf, "input buffer", mpi_errno);
	    }
	    /* Note: outbuf could be MPI_BOTTOM; don't test for NULL */
	    MPIR_ERRTEST_COUNT(insize, mpi_errno);
	    MPIR_ERRTEST_COUNT(outcount, mpi_errno);

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    /* If comm_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

	    if (datatype != MPI_DATATYPE_NULL &&
		HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype *datatype_ptr = NULL;

		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    position_x = *position;
    mpi_errno = MPIR_Unpack_impl(inbuf, insize, &position_x, outbuf, outcount, datatype);
    if (mpi_errno) goto fn_fail;
    MPIU_Assign_trunc(*position, position_x, int);
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPACK);
    return mpi_errno;


  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_unpack",
	    "**mpi_unpack %p %d %p %p %d %D %C", inbuf, insize, position, outbuf, outcount, datatype, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}












