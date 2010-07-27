/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_hindexed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_hindexed = PMPI_Type_create_hindexed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_hindexed  MPI_Type_create_hindexed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_hindexed as PMPI_Type_create_hindexed
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_hindexed
#define MPI_Type_create_hindexed PMPI_Type_create_hindexed

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_hindexed

/*@
   MPI_Type_create_hindexed - Create a datatype for an indexed datatype with
   displacements in bytes

   Input Parameters:
+ count - number of blocks --- also number of entries in
  displacements and blocklengths (integer)
. blocklengths - number of elements in each block (array of nonnegative integers)
. displacements - byte displacement of each block (array of address integers)
- oldtype - old datatype (handle)

   Output Parameter:
. newtype - new datatype (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_hindexed(int count,
			     int blocklengths[],
			     MPI_Aint displacements[],
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_hindexed";
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPID_Datatype *new_dtp;
    int i, *ints;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_HINDEXED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_HINDEXED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int j;
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(blocklengths, "blocklens", mpi_errno);
		MPIR_ERRTEST_ARGNULL(displacements, "indices", mpi_errno);
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_get_ptr(oldtype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    }
	    for (j=0; j < count; j++) {
		MPIR_ERRTEST_ARGNEG(blocklengths[j], "blocklen", mpi_errno);
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPID_Type_indexed(count,
				  blocklengths,
				  displacements,
				  1, /* displacements in bytes */
				  oldtype,
				  &new_handle);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_CHKLMEM_MALLOC_ORJUMP(ints, int *, (count + 1) * sizeof(int), mpi_errno, "content description");

    ints[0] = count;

    for (i=0; i < count; i++)
    {
	ints[i+1] = blocklengths[i];
    }
    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_HINDEXED,
				           count+1, /* ints (count, blocklengths) */
				           count, /* aints (displacements) */
				           1, /* types */
				           ints,
				           displacements,
				           &oldtype);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_HINDEXED);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_create_hindexed",
	    "**mpi_type_create_hindexed %d %p %p %D %p", count, blocklengths, displacements, oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
