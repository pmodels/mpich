/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Unpack
#define MPI_Unpack PMPI_Unpack

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Unpack

/*@
    MPI_Unpack - Unpack a buffer according to a datatype into contiguous memory

Input Parameters:
+ inbuf - input buffer start (choice) 
. insize - size of input buffer, in bytes (integer) 
. position - current position in bytes (integer) 
. outcount - number of items to be unpacked (integer) 
. datatype - datatype of each output data item (handle) 
- comm - communicator for packed message (handle) 

Output Parameter:
. outbuf - output buffer start (choice) 

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
int MPI_Unpack(void *inbuf, int insize, int *position,
	       void *outbuf, int outcount, MPI_Datatype datatype,
	       MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Unpack";
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;
    MPID_Comm *comm_ptr = NULL;
    MPID_Segment *segp;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_UNPACK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_UNPACK);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
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
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    
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
    if (insize == 0) {
	goto fn_exit;
    }
    
    segp = MPID_Segment_alloc();
    MPIU_ERR_CHKANDJUMP1((segp == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    MPID_Segment_init(outbuf, outcount, datatype, segp, 0);

    /* NOTE: the use of buffer values and positions in MPI_Unpack and in
     * MPID_Segment_unpack are quite different.  See code or docs or something.
     */
    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    MPID_Segment_unpack(segp,
			first,
			&last,
			(void *) ((char *) inbuf + *position));

    *position += (int) last;

    MPID_Segment_free(segp);

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
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}












