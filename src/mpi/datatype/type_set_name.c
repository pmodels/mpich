/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_set_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_set_name = PMPI_Type_set_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_set_name  MPI_Type_set_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_set_name as PMPI_Type_set_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_set_name
#define MPI_Type_set_name PMPI_Type_set_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_set_name
#undef FCNAME
#define FCNAME "MPI_Type_set_name"

/*@
   MPI_Type_set_name - set datatype name

   Input Parameters:
+ type - datatype whose identifier is to be set (handle) 
- type_name - the character string which is remembered as the name (string) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_TYPE
.N MPI_ERR_OTHER
@*/
int MPI_Type_set_name(MPI_Datatype type, char *type_name)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    static int setup = 0;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_SET_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_SET_NAME);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(type, "datatype", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Datatype_get_ptr( type, datatype_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int slen;
	    
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(type_name,"type_name", mpi_errno);
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    slen = (int)strlen( type_name );
	    MPIU_ERR_CHKANDSTMT1((slen >= MPI_MAX_OBJECT_NAME), mpi_errno, 
				 MPI_ERR_ARG,;, "**typenamelen",
				    "**typenamelen %d", slen );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* If this is the first call, initialize all of the predefined names.
       Note that type_get_name must also make the same call */
    if (!setup) { 
	MPIR_Datatype_init_names();
	setup = 1;
    }

    /* Include the null in MPI_MAX_OBJECT_NAME */
    MPIU_Strncpy( datatype_ptr->name, type_name, MPI_MAX_OBJECT_NAME );
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_SET_NAME);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_type_set_name",
	    "**mpi_type_set_name %D %s", type, type_name);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
