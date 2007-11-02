/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_hvector */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_hvector = PMPI_Type_hvector
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_hvector  MPI_Type_hvector
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_hvector as PMPI_Type_hvector
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_hvector
#define MPI_Type_hvector PMPI_Type_hvector

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_hvector

/*@
   MPI_Type_hvector - type_hvector

   Arguments:
+  int count - count
.  int blocklen - blocklen
.  MPI_Aint stride - stride
.  MPI_Datatype old_type - old datatype
-  MPI_Datatype *newtype - new datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_hvector(int count,
		     int blocklen,
		     MPI_Aint stride,
		     MPI_Datatype old_type,
		     MPI_Datatype *newtype_p)
{
    static const char FCNAME[] = "MPI_Type_hvector";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp;
    int ints[2];
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_HVECTOR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("datatype");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_HVECTOR);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNEG(blocklen,"blocklen",mpi_errno);
	    MPIR_ERRTEST_DATATYPE(old_type, "datatype", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
		if (HANDLE_GET_KIND(old_type) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype_get_ptr(old_type, datatype_ptr);
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		}
	    }
	    MPIR_ERRTEST_ARGNULL(newtype_p, "newtype", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Type_vector(count,
				 blocklen,
				 (MPI_Aint) stride,
				 1, /* stride in bytes */
				 old_type,
				 newtype_p);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    ints[0] = count;
    ints[1] = blocklen;
    MPID_Datatype_get_ptr(*newtype_p, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_HVECTOR,
    				           2, /* ints (count, blocklen) */
				           1, /* aints */
				           1, /* types */
				           ints,
				           &stride,
				           &old_type);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HVECTOR);
    MPIU_THREAD_SINGLE_CS_EXIT("datatype");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_hvector",
	    "**mpi_type_hvector %d %d %d %D %p", count, blocklen, stride, old_type, newtype_p);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

