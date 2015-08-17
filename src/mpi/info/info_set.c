/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

#include <string.h>

/* -- Begin Profiling Symbol Block for routine MPI_Info_set */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_set = PMPI_Info_set
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_set  MPI_Info_set
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_set as PMPI_Info_set
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Info_set(MPI_Info info, const char *key, const char *value) __attribute__((weak,alias("PMPI_Info_set")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_set
#define MPI_Info_set PMPI_Info_set
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_set

/*@
    MPI_Info_set - Adds a (key,value) pair to info

Input Parameters:
+ info - info object (handle)
. key - key (string)
- value - value (string)

.N NotThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO_KEY
.N MPI_ERR_INFO_VALUE
.N MPI_ERR_EXHAUSTED
@*/
int MPI_Info_set( MPI_Info info, const char *key, const char *value )
{
    static const char FCNAME[] = "MPI_Info_set";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_SET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); 
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_SET);
    
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
	    MPIR_ERR_CHKANDJUMP((!key), mpi_errno, MPI_ERR_INFO_KEY, "**infokeynull");
	    keylen = (int)strlen(key);
	    MPIR_ERR_CHKANDJUMP((keylen > MPI_MAX_INFO_KEY), mpi_errno, MPI_ERR_INFO_KEY, "**infokeylong");
	    MPIR_ERR_CHKANDJUMP((keylen == 0), mpi_errno, MPI_ERR_INFO_KEY, "**infokeyempty");

	    /* Check value arguments */
	    MPIR_ERR_CHKANDJUMP((!value), mpi_errno, MPI_ERR_INFO_VALUE, "**infovalnull");
	    MPIR_ERR_CHKANDJUMP((strlen(value) > MPI_MAX_INFO_VAL), mpi_errno, MPI_ERR_INFO_VALUE, "**infovallong");
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIR_Info_set_impl(info_ptr, key, value);
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_SET);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_info_set",
	    "**mpi_info_set %I %s %s", info, key, value);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#ifndef MPICH_MPI_FROM_PMPI

#undef FUNCNAME
#define FUNCNAME MPIR_Info_set_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Info_set_impl(MPID_Info *info_ptr, const char *key, const char *value)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *curr_ptr, *prev_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_INFO_SET_IMPL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_INFO_SET_IMPL);

    prev_ptr = info_ptr;
    curr_ptr = info_ptr->next;

    while (curr_ptr) {
        if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
            /* Key already present; replace value */
            MPIU_Free(curr_ptr->value);
            curr_ptr->value = MPIU_Strdup(value);
            break;
        }
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    if (!curr_ptr) {
        /* Key not present, insert value */
        mpi_errno = MPIU_Info_alloc(&curr_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /*printf( "Inserting new elm %x at %x\n", curr_ptr->id, prev_ptr->id );*/
        prev_ptr->next   = curr_ptr;
        curr_ptr->key    = MPIU_Strdup(key);
        curr_ptr->value  = MPIU_Strdup(value);
    }

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_INFO_SET_IMPL);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */
