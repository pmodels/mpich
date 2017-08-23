/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_datatype.h"
#include "mpir_dataloop.h"
#include "datatype.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_contents */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_contents = PMPI_Type_get_contents
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_contents  MPI_Type_get_contents
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_contents as PMPI_Type_get_contents
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses,
                          int max_datatypes, int array_of_integers[],
                          MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]) __attribute__((weak,alias("PMPI_Type_get_contents")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_contents
#define MPI_Type_get_contents PMPI_Type_get_contents

#endif

/*@
  MPIR_Type_get_contents - get content information from datatype

Input Parameters:
+ datatype - MPI datatype
. max_integers - size of array_of_integers
. max_addresses - size of array_of_addresses
- max_datatypes - size of array_of_datatypes

Output Parameters:
+ array_of_integers - integers used in creating type
. array_of_addresses - MPI_Aints used in creating type
- array_of_datatypes - MPI_Datatypes used in creating type

@*/
int MPIR_Type_get_contents(MPI_Datatype datatype,
                           int max_integers,
                           int max_addresses,
                           int max_datatypes,
                           int array_of_integers[],
                           MPI_Aint array_of_addresses[],
                           MPI_Datatype array_of_datatypes[])
{
    int i, mpi_errno;
    MPIR_Datatype *dtp;
    MPIR_Datatype_contents *cp;

    /* --BEGIN ERROR HANDLING-- */
    /* these are checked at the MPI layer, so I feel that asserts
     * are appropriate.
     */
    MPIR_Assert(HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN);
    MPIR_Assert(datatype != MPI_FLOAT_INT &&
                datatype != MPI_DOUBLE_INT &&
                datatype != MPI_LONG_INT &&
                datatype != MPI_SHORT_INT &&
                datatype != MPI_LONG_DOUBLE_INT);
    /* --END ERROR HANDLING-- */

    MPIR_Datatype_get_ptr(datatype, dtp);
    cp = dtp->contents;
    MPIR_Assert(cp != NULL);

    /* --BEGIN ERROR HANDLING-- */
    if (max_integers < cp->nr_ints ||
        max_addresses < cp->nr_aints ||
        max_datatypes < cp->nr_types)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_get_contents", __LINE__,
                                         MPI_ERR_OTHER, "**dtype", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (cp->nr_ints > 0)
    {
        MPII_Datatype_get_contents_ints(cp, array_of_integers);
    }

    if (cp->nr_aints > 0) {
        MPII_Datatype_get_contents_aints(cp, array_of_addresses);
    }

    if (cp->nr_types > 0) {
        MPII_Datatype_get_contents_types(cp, array_of_datatypes);
    }

    for (i=0; i < cp->nr_types; i++)
    {
        if (HANDLE_GET_KIND(array_of_datatypes[i]) != HANDLE_KIND_BUILTIN)
        {
            MPIR_Datatype_get_ptr(array_of_datatypes[i], dtp);
            MPIR_Datatype_add_ref(dtp);
        }
    }

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_contents

/*@
   MPI_Type_get_contents - get type contents

Input Parameters:
+  datatype - datatype to access (handle)
.  max_integers - number of elements in array_of_integers (non-negative integer)
.  max_addresses - number of elements in array_of_addresses (non-negative integer)
-  max_datatypes - number of elements in array_of_datatypes (non-negative integer)

Output Parameters:
+  array_of_integers - contains integer arguments used in constructing the datatype (array of integers)
.  array_of_addresses - contains address arguments used in constructing the datatype (array of integers)
-  array_of_datatypes - contains datatype arguments used in constructing the datatype (array of handles)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_get_contents(MPI_Datatype datatype,
			  int max_integers,
			  int max_addresses,
			  int max_datatypes,
			  int array_of_integers[],
			  MPI_Aint array_of_addresses[],
			  MPI_Datatype array_of_datatypes[])
{
    static const char FCNAME[] = "MPI_Type_get_contents";
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_GET_CONTENTS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_GET_CONTENTS);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *datatype_ptr = NULL;

	    /* Check for built-in type */
	    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_RECOVERABLE,
						 FCNAME, __LINE__,
						 MPI_ERR_TYPE,
						 "**contentspredef", 0);
		goto fn_fail;
	    }

	    /* all but MPI_2INT of the pair types are not stored as builtins
	     * but should be treated similarly.
	     */
	    if (datatype == MPI_FLOAT_INT ||
		datatype == MPI_DOUBLE_INT ||
		datatype == MPI_LONG_INT ||
		datatype == MPI_SHORT_INT ||
		datatype == MPI_LONG_DOUBLE_INT)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_RECOVERABLE,
						 FCNAME, __LINE__,
						 MPI_ERR_TYPE,
						 "**contentspredef", 0);
		goto fn_fail;
	    }

            /* Convert MPI object handles to object pointers */
            MPIR_Datatype_get_ptr(datatype, datatype_ptr);

            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Type_get_contents(datatype,
				       max_integers,
				       max_addresses,
				       max_datatypes,
				       array_of_integers,
				       array_of_addresses,
				       array_of_datatypes);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_GET_CONTENTS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_get_contents",
	    "**mpi_type_get_contents %D %d %d %d %p %p %p", datatype, max_integers, max_addresses, max_datatypes,
	    array_of_integers, array_of_addresses, array_of_datatypes);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



