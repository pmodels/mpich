/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Grequest_start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Grequest_start = PMPI_Grequest_start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Grequest_start  MPI_Grequest_start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Grequest_start as PMPI_Grequest_start
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
                       MPI_Grequest_cancel_function *cancel_fn, void *extra_state,
                       MPI_Request *request) __attribute__((weak,alias("PMPI_Grequest_start")));
#endif
/* -- End Profiling Symbol Block */

PMPI_LOCAL int MPIR_Grequest_free_classes_on_finalize(void *extra_data);

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Grequest_start
#define MPI_Grequest_start PMPI_Grequest_start

/* preallocated grequest classes */
#ifndef MPID_GREQ_CLASS_PREALLOC
#define MPID_GREQ_CLASS_PREALLOC 2
#endif

MPID_Grequest_class MPID_Grequest_class_direct[MPID_GREQ_CLASS_PREALLOC] = 
                                              { {0} };
MPIU_Object_alloc_t MPID_Grequest_class_mem = {0, 0, 0, 0, MPID_GREQ_CLASS,
	                                       sizeof(MPID_Grequest_class),
					       MPID_Grequest_class_direct,
					       MPID_GREQ_CLASS_PREALLOC, };

/* We jump through some minor hoops to manage the list of classes ourselves and
 * only register a single finalizer to avoid hitting limitations in the current
 * finalizer code.  If the finalizer implementation is ever revisited this code
 * is a good candidate for registering one callback per greq class and trimming
 * some of this logic. */
int MPIR_Grequest_registered_finalizer = 0;
MPID_Grequest_class *MPIR_Grequest_class_list = NULL;

/* Any internal routines can go here.  Make them static if possible.  If they
   are used by both the MPI and PMPI versions, use PMPI_LOCAL instead of 
   static; this macro expands into "static" if weak symbols are supported and
   into nothing otherwise. */
PMPI_LOCAL int MPIR_Grequest_free_classes_on_finalize(void *extra_data ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Grequest_class *last = NULL;
    MPID_Grequest_class *cur = MPIR_Grequest_class_list;

    /* FIXME MT this function is not thread safe when using fine-grained threading */
    MPIR_Grequest_class_list = NULL;
    while (cur) {
        last = cur;
        cur = last->next;
        MPIU_Handle_obj_free(&MPID_Grequest_class_mem, last);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Grequest_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Grequest_start_impl(MPI_Grequest_query_function *query_fn,
                             MPI_Grequest_free_function *free_fn,
                             MPI_Grequest_cancel_function *cancel_fn,
                             void *extra_state, MPID_Request **request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);

    /* MT FIXME this routine is not thread-safe in the non-global case */
    
    *request_ptr = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP1(request_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "generalized request");
    
    (*request_ptr)->kind                 = MPID_UREQUEST;
    MPIU_Object_set_ref( *request_ptr, 1 );
    (*request_ptr)->cc_ptr               = &(*request_ptr)->cc;
    MPID_cc_set((*request_ptr)->cc_ptr, 1);
    (*request_ptr)->comm                 = NULL;
    (*request_ptr)->greq_fns             = NULL;
    MPIU_CHKPMEM_MALLOC((*request_ptr)->greq_fns, struct MPID_Grequest_fns *, sizeof(struct MPID_Grequest_fns), mpi_errno, "greq_fns");
    (*request_ptr)->greq_fns->cancel_fn            = cancel_fn;
    (*request_ptr)->greq_fns->free_fn              = free_fn;
    (*request_ptr)->greq_fns->query_fn             = query_fn;
    (*request_ptr)->greq_fns->poll_fn              = NULL;
    (*request_ptr)->greq_fns->wait_fn              = NULL;
    (*request_ptr)->greq_fns->grequest_extra_state = extra_state;
    (*request_ptr)->greq_fns->greq_lang            = MPID_LANG_C;

    /* Add an additional reference to the greq.  One of them will be
     * released when we complete the request, and the second one, when
     * we test or wait on it. */
    MPIR_Request_add_ref((*request_ptr));

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#else
extern MPID_Grequest_class MPID_Grequest_class_direct[];
extern MPIU_Object_alloc_t MPID_Grequest_class_mem;
extern int MPIR_Grequest_registered_finalizer;
extern MPID_Grequest_class *MPIR_Grequest_class_list;
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Grequest_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Grequest_start - Create and return a user-defined request

Input Parameters:
+ query_fn - callback function invoked when request status is queried (function)  
. free_fn - callback function invoked when request is freed (function) 
. cancel_fn - callback function invoked when request is cancelled (function) 
- extra_state - Extra state passed to the above functions.

Output Parameters:
.  request - Generalized request (handle)

 Notes on the callback functions:
 The return values from the callback functions must be a valid MPI error code
 or class.  This value may either be the return value from any MPI routine
 (with one exception noted below) or any of the MPI error classes. 
 For portable programs, 'MPI_ERR_OTHER' may be used; to provide more 
 specific information, create a new MPI error class or code with 
 'MPI_Add_error_class' or 'MPI_Add_error_code' and return that value.

 The MPI standard is not clear on the return values from the callback routines.
 However, there are notes in the standard that imply that these are MPI error
 codes.  For example, pages 169 line 46 through page 170, line 1 require that
 the 'free_fn' return an MPI error code that may be used in the MPI completion
 functions when they return 'MPI_ERR_IN_STATUS'.  

 The one special case is the error value returned by 'MPI_Comm_dup' when
 the attribute callback routine returns a failure.  The MPI standard is not
 clear on what values may be used to indicate an error return.  Further,
 the Intel MPI test suite made use of non-zero values to indicate failure, 
 and expected these values to be returned by the 'MPI_Comm_dup' when the 
 attribute routines encountered an error.  Such error values may not be valid 
 MPI error codes or classes.  Because of this, it is the user''s responsibility
 to either use valid MPI error codes in return from the attribute callbacks,
 if those error codes are to be returned by a generalized request callback,
 or to detect and convert those error codes to valid MPI error codes (recall
 that MPI error classes are valid error codes).  

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Grequest_start( MPI_Grequest_query_function *query_fn, 
			MPI_Grequest_free_function *free_fn, 
			MPI_Grequest_cancel_function *cancel_fn, 
			void *extra_state, MPI_Request *request )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GREQUEST_START);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GREQUEST_START);

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Grequest_start_impl(query_fn, free_fn, cancel_fn, extra_state, &request_ptr);
    if (mpi_errno) goto fn_fail;
    
    MPID_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GREQUEST_START);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_grequest_start",
	    "**mpi_grequest_start %p %p %p %p %p", 
	    query_fn, free_fn, cancel_fn, extra_state, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* -- Begin Profiling Symbol Block for routine MPIX_Grequest_class_create*/
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Grequest_class_create = PMPIX_Grequest_class_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Grequest_class_create MPIX_Grequest_class_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Grequest_class_create as PMPIX_Grequest_class_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn,
        MPI_Grequest_free_function *free_fn,
        MPI_Grequest_cancel_function *cancel_fn,
        MPIX_Grequest_poll_function *poll_fn,
        MPIX_Grequest_wait_function *wait_fn,
        MPIX_Grequest_class *greq_class) __attribute__((weak,alias("PMPIX_Grequest_class_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Grequest_class_create
#define MPIX_Grequest_class_create PMPIX_Grequest_class_create
#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Grequest_class_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* extensions for Generalized Request redesign paper */
int MPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn,
		               MPI_Grequest_free_function *free_fn,
			       MPI_Grequest_cancel_function *cancel_fn,
			       MPIX_Grequest_poll_function *poll_fn,
			       MPIX_Grequest_wait_function *wait_fn,
			       MPIX_Grequest_class *greq_class)
{
	MPID_Grequest_class *class_ptr;
	int mpi_errno = MPI_SUCCESS;

	class_ptr = (MPID_Grequest_class *) 
		MPIU_Handle_obj_alloc(&MPID_Grequest_class_mem);
        /* --BEGIN ERROR HANDLING-- */
	if (!class_ptr)
	{
	    mpi_errno = MPIR_Err_create_code (MPI_SUCCESS, 
			    MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
			    MPI_ERR_OTHER, "**nomem", 
			    "**nomem %s", "MPIX_Grequest_class");
	    goto fn_fail;
	} 
	/* --END ERROR HANDLING-- */

	class_ptr->query_fn = query_fn;
	class_ptr->free_fn = free_fn;
	class_ptr->cancel_fn = cancel_fn;
	class_ptr->poll_fn = poll_fn;
	class_ptr->wait_fn = wait_fn;

	MPIU_Object_set_ref(class_ptr, 1);

        if (MPIR_Grequest_class_list == NULL) {
            class_ptr->next = NULL;
        }
        else {
            class_ptr->next = MPIR_Grequest_class_list;
        }
        MPIR_Grequest_class_list = class_ptr;
        if (!MPIR_Grequest_registered_finalizer) {
            /* must run before (w/ higher priority than) the handle check
             * finalizer in order avoid being flagged as a leak */
            MPIR_Add_finalize(&MPIR_Grequest_free_classes_on_finalize,
                              NULL,
                              MPIR_FINALIZE_CALLBACK_HANDLE_CHECK_PRIO+1);
            MPIR_Grequest_registered_finalizer = 1;
        }

        MPID_OBJ_PUBLISH_HANDLE(*greq_class, class_ptr->handle);

	/* ... end of body of routine ... */
fn_exit:
	return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpix_grequest_class_create",
	    "**mpix_grequest_class_create %p %p %p %p %p", 
	    query_fn, free_fn, cancel_fn, poll_fn, wait_fn);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* -- Begin Profiling Symbol Block for routine MPIX_Grequest_class_allocate */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Grequest_class_allocate = PMPIX_Grequest_class_allocate
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Grequest_class_allocate MPIX_Grequest_class_allocate
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Grequest_class_allocate as PMPIX_Grequest_class_allocate
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state, MPI_Request *request) __attribute__((weak,alias("PMPIX_Grequest_class_allocate")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Grequest_class_allocate
#define MPIX_Grequest_class_allocate PMPIX_Grequest_class_allocate
#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Grequest_class_allocate

int MPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, 
		                void *extra_state, 
				MPI_Request *request)
{
	int mpi_errno;
	MPID_Request *lrequest_ptr;
	MPID_Grequest_class *class_ptr;

        *request = MPI_REQUEST_NULL;
	MPID_Grequest_class_get_ptr(greq_class, class_ptr);
        mpi_errno = MPIR_Grequest_start_impl(class_ptr->query_fn, class_ptr->free_fn,
                                             class_ptr->cancel_fn, extra_state,
                                             &lrequest_ptr);
	if (mpi_errno == MPI_SUCCESS)
	{
            *request = lrequest_ptr->handle;
            lrequest_ptr->greq_fns->poll_fn     = class_ptr->poll_fn;
            lrequest_ptr->greq_fns->wait_fn     = class_ptr->wait_fn;
            lrequest_ptr->greq_fns->greq_class  = greq_class;
	}
	return mpi_errno;
}

/* -- Begin Profiling Symbol Block for routine MPIX_Grequest_start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Grequest_start = PMPIX_Grequest_start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Grequest_start MPIX_Grequest_start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Grequest_start as PMPIX_Grequest_start
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Grequest_start( MPI_Grequest_query_function *query_fn,
        MPI_Grequest_free_function *free_fn,
        MPI_Grequest_cancel_function *cancel_fn,
        MPIX_Grequest_poll_function *poll_fn,
        MPIX_Grequest_wait_function *wait_fn,
        void *extra_state,
        MPI_Request *request ) __attribute__((weak,alias("PMPIX_Grequest_start")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Grequest_start
#define MPIX_Grequest_start PMPIX_Grequest_start
#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Grequest_start

int MPIX_Grequest_start( MPI_Grequest_query_function *query_fn,
			MPI_Grequest_free_function *free_fn,
			MPI_Grequest_cancel_function *cancel_fn,
			MPIX_Grequest_poll_function *poll_fn,
			MPIX_Grequest_wait_function *wait_fn,
			void *extra_state, MPI_Request *request )
{
    int mpi_errno;
    MPID_Request *lrequest_ptr;

    *request = MPI_REQUEST_NULL;
    mpi_errno = MPIX_Grequest_start_impl(query_fn, free_fn, cancel_fn, poll_fn, wait_fn, extra_state, &lrequest_ptr);

    if (mpi_errno == MPI_SUCCESS) {
        *request = lrequest_ptr->handle;
    }

    return mpi_errno;
}


#ifndef MPICH_MPI_FROM_PMPI

#undef FUNCNAME
#define FUNCNAME MPIX_Grequest_start_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIX_Grequest_start_impl( MPI_Grequest_query_function *query_fn,
                              MPI_Grequest_free_function *free_fn,
                              MPI_Grequest_cancel_function *cancel_fn,
                              MPIX_Grequest_poll_function *poll_fn,
                              MPIX_Grequest_wait_function *wait_fn,
                              void *extra_state, MPID_Request **request )
{
    int mpi_errno;

    mpi_errno = MPIR_Grequest_start_impl(query_fn, free_fn, cancel_fn, extra_state, request);

    if (mpi_errno == MPI_SUCCESS) {
        (*request)->greq_fns->poll_fn = poll_fn;
        (*request)->greq_fns->wait_fn = wait_fn;
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */
