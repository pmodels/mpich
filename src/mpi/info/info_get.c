/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get = PMPI_Info_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get  MPI_Info_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get as PMPI_Info_get
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag) __attribute__((weak,alias("PMPI_Info_get")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_get
#define MPI_Info_get PMPI_Info_get

#undef FUNCNAME
#define FUNCNAME MPIR_Info_get_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Info_get_impl(MPID_Info *info_ptr, const char *key, int valuelen, char *value, int *flag)
{
    MPID_Info *curr_ptr;
    int err=0, mpi_errno=0;

    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            err = MPIU_Strncpy(value, curr_ptr->value, valuelen+1);
            /* +1 because the MPI Standard says "In C, valuelen
             * (passed to MPI_Info_get) should be one less than the
             * amount of allocated space to allow for the null
             * terminator*/
            *flag = 1;
            break;
        }
        curr_ptr = curr_ptr->next;
    }

    /* --BEGIN ERROR HANDLING-- */
    if (err != 0)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_INFO_VALUE, "**infovallong", NULL);
    }
    /* --END ERROR HANDLING-- */

    return mpi_errno;
}

#endif

/*@
    MPI_Info_get - Retrieves the value associated with a key

Input Parameters:
+ info - info object (handle)
. key - key (string)
- valuelen - length of value argument, not including null terminator (integer)

Output Parameters:
+ value - value (string)
- flag - true if key defined, false if not (boolean)


.N ThreadSafeInfoRead

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
.N MPI_ERR_INFO_KEY
.N MPI_ERR_ARG
.N MPI_ERR_INFO_VALUE
@*/
#undef FUNCNAME
#define FUNCNAME MPI_Info_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value,
		 int *flag)
{
    MPID_Info *info_ptr=0;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_GET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INFO(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Info_get_ptr( info, info_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int keylen;
	    
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
	    
	    /* Check key */
	    MPIR_ERR_CHKANDJUMP((!key), mpi_errno, MPI_ERR_INFO_KEY, 
				"**infokeynull");
	    keylen = (int)strlen(key);
	    MPIR_ERR_CHKANDJUMP((keylen > MPI_MAX_INFO_KEY), mpi_errno, 
				MPI_ERR_INFO_KEY, "**infokeylong");
	    MPIR_ERR_CHKANDJUMP((keylen == 0), mpi_errno, MPI_ERR_INFO_KEY, 
				"**infokeyempty");

	    /* Check value arguments */
	    MPIR_ERRTEST_ARGNEG(valuelen, "valuelen", mpi_errno);
	    MPIR_ERR_CHKANDSTMT((!value), mpi_errno, MPI_ERR_INFO_VALUE,goto fn_fail,
				"**infovalnull");
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Info_get_impl(info_ptr, key, valuelen, value, flag);
    /* ... end of body of routine ... */
    if (mpi_errno) goto fn_fail;

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE,
	    FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_info_get",
	    "**mpi_info_get %I %s %d %p %p", info, key, valuelen, value, flag);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
#   endif
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
