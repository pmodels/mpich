/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style: allow:fprintf:4 sig:0 */

/* stdarg is required to handle the variable argument lists for 
   MPIR_Err_create_code */
#include <stdarg.h>
/* Define USE_ERR_CODE_VALIST to get the prototype for the valist version
   of MPIR_Err_create_code in mpir_err.h (without this definition,
   the prototype is not included.  The "valist" version of the function
   is used in only a few places, here and potentially in ROMIO) */
#define USE_ERR_CODE_VALIST

#include "mpiimpl.h"
/* errcodes.h contains the macros used to access fields within an error
   code and a description of the bits in an error code.  A brief
   version of that description is included below */

#include "errcodes.h"

/* defmsg is generated automatically from the source files and contains
   all of the error messages, both the generic and specific.  Depending 
   on the value of MPICH_ERROR_MSG_LEVEL, different amounts of message
   information will be included from the file defmsg.h */
#include "defmsg.h" 

/* stdio is needed for vsprintf and vsnprintf */
#include <stdio.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : ERROR_HANDLING
      description : cvars that control error handling behavior (stack traces, aborts, etc)

cvars:
    - name        : MPIR_CVAR_PRINT_ERROR_STACK
      category    : ERROR_HANDLING
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, print an error stack trace at error handling time.

    - name        : MPIR_CVAR_CHOP_ERROR_STACK
      category    : ERROR_HANDLING
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If >0, truncate error stack output lines this many characters
        wide.  If 0, do not truncate, and if <0 use a sensible default.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/*
 * Structure of this file
 *
 * This file contains several groups of routines user for error handling
 * and reporting.  
 *
 * The first group provides memory for the MPIR_Errhandler objects
 * and the routines to free and manipulate them
 *
 * The second group provides routines to call the appropriate error handler,
 * which may be predefined or user defined.  These also return the 
 * appropriate return code.  These routines have names of the form
 * MPIR_Err_return_xxx.  Specifically, for each of the MPI types on which an 
 * error handler can be defined, there is an MPIR_Err_return_xxx routine
 * that determines what error handler function to call and whether to 
 * abort the program.  The comm and win versions are here; ROMIO
 * provides its own routines for invoking the error handlers for Files.
 *
 * The third group of code handles the error messages.  There are four
 * options, controlled by the value of MPICH_ERROR_MSG_LEVEL. 
 *
 * MPICH_ERROR_MSG__NONE - No text messages at all
 * MPICH_ERROR_MSG__CLASS - Only messages for the MPI error classes
 * MPICH_ERROR_MSG__GENERIC - Only predefiend messages for the MPI error codes
 * MPICH_ERROR_MSG__ALL - Instance specific error messages (and error message
 *                       stack)
 *
 * In only the latter (MPICH_ERROR_MSG__ALL) case are instance-specific
 * messages maintained (including the error message "stack" that you may
 * see mentioned in various places.  In the other cases, an error code 
 * identifies a fixed message string (unless MPICH_ERROR_MSG__NONE,
 * when there are no strings) from the "generic" strings defined in defmsg.h
 *
 * A major subgroup in this section is the code to handle the instance-specific
 * messages (MPICH_ERROR_MSG__ALL only).
 *
 * An MPI error code is made up of a number of fields (see errcodes.h)
 * These ar 
 *   is-dynamic? specific-msg-sequence# specific-msg-index 
 *                                            generic-code is-fatal? class
 *
 * There are macros (defined in errcodes.h) that define these fields, 
 * their sizes, and masks and shifts that may be used to extract them.
 */

static int did_err_init = FALSE; /* helps us solve a bootstrapping problem */

/* A few prototypes.  These routines are called from the MPIR_Err_return 
   routines.  checkValidErrcode depends on the MPICH_ERROR_MSG_LEVEL.
   If the error code is *not* valid, checkValidErrcode may replace it 
   with a valid value.  */

static int checkValidErrcode( int, const char [], int * );

#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG__ALL
static int ErrGetInstanceString( int, char [], int );
static void MPIR_Err_stack_init( void );
static int checkForUserErrcode( int );
#else
/* We only need special handling for user error codes when we support the
   error message stack */
#define checkForUserErrcode(_a) _a
#endif /* ERROR_MSG_LEVEL >= ERROR_MSG_ALL */


/* ------------------------------------------------------------------------- */
/* Provide the MPIR_Errhandler space and the routines to free and set them
   from C++ and Fortran */
/* ------------------------------------------------------------------------- */
/*
 * Error handlers.  These are handled just like the other opaque objects
 * in MPICH
 */

#ifndef MPIR_ERRHANDLER_PREALLOC
#define MPIR_ERRHANDLER_PREALLOC 8
#endif

/* Preallocated errorhandler objects */
MPIR_Errhandler MPIR_Errhandler_builtin[3] = { {0} };
MPIR_Errhandler MPIR_Errhandler_direct[MPIR_ERRHANDLER_PREALLOC] =
    { {0} };
MPIR_Object_alloc_t MPIR_Errhandler_mem = { 0, 0, 0, 0, MPIR_ERRHANDLER,
					    sizeof(MPIR_Errhandler),
					    MPIR_Errhandler_direct,
					    MPIR_ERRHANDLER_PREALLOC, };

void MPIR_Errhandler_free(MPIR_Errhandler *errhan_ptr)
{
    MPIR_Handle_obj_free(&MPIR_Errhandler_mem, errhan_ptr);
}

void MPIR_Err_init( void )
{
    /* these are "stub" objects, so the other fields (which are statically
     * initialized to zero) don't really matter */
    MPIR_Errhandler_builtin[0].handle = MPI_ERRORS_ARE_FATAL;
    MPIR_Errhandler_builtin[1].handle = MPI_ERRORS_RETURN;
    MPIR_Errhandler_builtin[2].handle = MPIR_ERRORS_THROW_EXCEPTIONS;

#   if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG__ALL
    MPIR_Err_stack_init();
#   endif
    did_err_init = TRUE;
}

/* Language Callbacks */

#ifdef HAVE_CXX_BINDING
/* This routine is used to install a callback used by the C++ binding
 to invoke the (C++) error handler.  The callback routine is a C routine,
 defined in the C++ binding. */
void MPII_Errhandler_set_cxx( MPI_Errhandler errhand, void (*errcall)(void) )
{
    MPIR_Errhandler *errhand_ptr;
    
    MPIR_Errhandler_get_ptr( errhand, errhand_ptr );
    errhand_ptr->language		= MPIR_LANG__CXX;
    MPIR_Process.cxx_call_errfn	= (void (*)( int, int *, int *, 
					    void (*)(void) ))errcall;
}
#endif /* HAVE_CXX_BINDING */

#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
void MPII_Errhandler_set_fc( MPI_Errhandler errhand )
{
    MPIR_Errhandler *errhand_ptr;
    
    MPIR_Errhandler_get_ptr( errhand, errhand_ptr );
    errhand_ptr->language = MPIR_LANG__FORTRAN;
}

#endif

/* ------------------------------------------------------------------------- */
/* Group 2: These routines are called on error exit from most
   top-level MPI routines to invoke the appropriate error handler.
   Also included is the routine to call if MPI has not been
   initialized (MPIR_Err_preinit) and to determine if an error code
   represents a fatal error (MPIR_Err_is_fatal). */
/* ------------------------------------------------------------------------- */
/* Special error handler to call if we are not yet initialized, or if we
   have finalized */
/* --BEGIN ERROR HANDLING-- */
void MPIR_Err_preOrPostInit( void )
{
    if (OPA_load_int(&MPIR_Process.mpich_state) == MPICH_MPI_STATE__PRE_INIT) {
	MPL_error_printf("Attempting to use an MPI routine before initializing MPICH\n");
    }
    else if (OPA_load_int(&MPIR_Process.mpich_state) == MPICH_MPI_STATE__POST_FINALIZED) {
	MPL_error_printf("Attempting to use an MPI routine after finalizing MPICH\n");
    }
    else {
	MPL_error_printf("Internal Error: Unknown state of MPI (neither initialized nor finalized)\n" );
    }
    exit(1);
}
/* --END ERROR HANDLING-- */

/* Return true if the error code indicates a fatal error */
int MPIR_Err_is_fatal(int errcode)
{
    return (errcode & ERROR_FATAL_MASK) ? TRUE : FALSE;
}

/*
 * This is the routine that is invoked by most MPI routines to 
 * report an error.  It is legitimate to pass NULL for comm_ptr in order to get
 * the default (MPI_COMM_WORLD) error handling.
 */
int MPIR_Err_return_comm( MPIR_Comm  *comm_ptr, const char fcname[],
			  int errcode )
{
    const int error_class = ERROR_GET_CLASS(errcode);
    MPIR_Errhandler *errhandler = NULL;

    checkValidErrcode( error_class, fcname, &errcode );

    /* --BEGIN ERROR HANDLING-- */
    if (OPA_load_int(&MPIR_Process.mpich_state) == MPICH_MPI_STATE__PRE_INIT ||
        OPA_load_int(&MPIR_Process.mpich_state) == MPICH_MPI_STATE__POST_FINALIZED)
    {
        /* for whatever reason, we aren't initialized (perhaps error 
	   during MPI_Init) */
        MPIR_Handle_fatal_error(MPIR_Process.comm_world, fcname, errcode);
        return MPI_ERR_INTERN;
    }
    /* --END ERROR HANDLING-- */

    MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND, TERSE, (MPL_DBG_FDEST, "MPIR_Err_return_comm(comm_ptr=%p, fcname=%s, errcode=%d)", comm_ptr, fcname, errcode));

    if (comm_ptr) {
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
        errhandler = comm_ptr->errhandler;
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    }

    if (errhandler == NULL) {
        /* Try to replace with the default handler, which is the one on 
           MPI_COMM_WORLD.  This gives us correct behavior for the
           case where the error handler on MPI_COMM_WORLD has been changed. */
        if (MPIR_Process.comm_world)
        {
            comm_ptr = MPIR_Process.comm_world;
        }
    }

    /* --BEGIN ERROR HANDLING-- */
    if (MPIR_Err_is_fatal(errcode) || comm_ptr == NULL) {
	/* Calls MPID_Abort */
	MPIR_Handle_fatal_error( comm_ptr, fcname, errcode );
        /* never get here */
    }
    /* --END ERROR HANDLING-- */

    MPIR_Assert(comm_ptr != NULL);

    /* comm_ptr may have changed to comm_world.  Keep this locked as long as we
     * are using the errhandler to prevent it from disappearing out from under
     * us. */
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    errhandler = comm_ptr->errhandler;

    /* --BEGIN ERROR HANDLING-- */
    if (errhandler == NULL || errhandler->handle == MPI_ERRORS_ARE_FATAL) {
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
	/* Calls MPID_Abort */
	MPIR_Handle_fatal_error( comm_ptr, fcname, errcode );
        /* never get here */
    }
    /* --END ERROR HANDLING-- */

    /* Check for the special case of a user-provided error code */
    errcode = checkForUserErrcode( errcode );

    if (errhandler->handle != MPI_ERRORS_RETURN &&
        errhandler->handle != MPIR_ERRORS_THROW_EXCEPTIONS)
    {
	/* We pass a final 0 (for a null pointer) to these routines
	   because MPICH-1 expected that */
	switch (comm_ptr->errhandler->language)
	{
	case MPIR_LANG__C:
	    (*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&comm_ptr->handle, &errcode, 0 );
	    break;
#ifdef HAVE_CXX_BINDING
	case MPIR_LANG__CXX:
	    (*MPIR_Process.cxx_call_errfn)( 0, &comm_ptr->handle, &errcode, 
		    (void (*)(void))*comm_ptr->errhandler->errfn.C_Comm_Handler_function );
	    /* The C++ code throws an exception if the error handler 
	     returns something other than MPI_SUCCESS. There is no "return"
	     of an error code. */
	    errcode = MPI_SUCCESS;
	    break;
#endif /* CXX_BINDING */
#ifdef HAVE_FORTRAN_BINDING
	case MPIR_LANG__FORTRAN90:
	case MPIR_LANG__FORTRAN:
	{
	    /* If int and MPI_Fint aren't the same size, we need to 
	       convert.  As this is not performance critical, we
	       do this even if MPI_Fint and int are the same size. */
	    MPI_Fint ferr=errcode;
	    MPI_Fint commhandle=comm_ptr->handle;
	    (*comm_ptr->errhandler->errfn.F77_Handler_function)( &commhandle, 
								 &ferr );
	}
	    break;
#endif /* FORTRAN_BINDING */
	}

    }

    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
    return errcode;
}

/* 
 * MPI routines that detect errors on window objects use this to report errors
 */
int MPIR_Err_return_win( MPIR_Win  *win_ptr, const char fcname[], int errcode )
{
    const int error_class = ERROR_GET_CLASS(errcode);

    if (win_ptr == NULL || win_ptr->errhandler == NULL)
	return MPIR_Err_return_comm(NULL, fcname, errcode);

    /* We don't test for MPI initialized because to call this routine,
       we will have had to call an MPI routine that would make that test */

    checkValidErrcode( error_class, fcname, &errcode );

    MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND, TERSE, (MPL_DBG_FDEST, "MPIR_Err_return_win(win_ptr=%p, fcname=%s, errcode=%d)", win_ptr, fcname, errcode));

    /* --BEGIN ERROR HANDLING-- */
    if (MPIR_Err_is_fatal(errcode) ||
	win_ptr == NULL || win_ptr->errhandler == NULL || 
	win_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
	/* Calls MPID_Abort */
	MPIR_Handle_fatal_error( NULL, fcname, errcode );
    }
    /* --END ERROR HANDLING-- */

    /* Check for the special case of a user-provided error code */
    errcode = checkForUserErrcode( errcode );
    
    if (win_ptr->errhandler->handle == MPI_ERRORS_RETURN ||
	win_ptr->errhandler->handle == MPIR_ERRORS_THROW_EXCEPTIONS)
    {
	return errcode;
    }
    else
    {
	/* Now, invoke the error handler for the window */

	/* We pass a final 0 (for a null pointer) to these routines
	   because MPICH-1 expected that */
	switch (win_ptr->errhandler->language)
	{
	    case MPIR_LANG__C:
		(*win_ptr->errhandler->errfn.C_Win_Handler_function)( 
		    &win_ptr->handle, &errcode, 0 );
		break;
#ifdef HAVE_CXX_BINDING
	    case MPIR_LANG__CXX:
	    (*MPIR_Process.cxx_call_errfn)( 2, &win_ptr->handle, &errcode, 
		    (void (*)(void))*win_ptr->errhandler->errfn.C_Win_Handler_function );
	    /* The C++ code throws an exception if the error handler 
	     returns something other than MPI_SUCCESS. There is no "return"
	     of an error code. */
	    errcode = MPI_SUCCESS;
	    break;
#endif /* CXX_BINDING */
#ifdef HAVE_FORTRAN_BINDING
	    case MPIR_LANG__FORTRAN90:
	    case MPIR_LANG__FORTRAN:
		{
		    /* If int and MPI_Fint aren't the same size, we need to 
		       convert.  As this is not performance critical, we
		       do this even if MPI_Fint and int are the same size. */
		    MPI_Fint ferr=errcode;
		    MPI_Fint winhandle=win_ptr->handle;
		    (*win_ptr->errhandler->errfn.F77_Handler_function)( 
						       &winhandle, &ferr );
		}
		break;
#endif /* FORTRAN_BINDING */
	}

    }
    return errcode;
}

/* ------------------------------------------------------------------------- */
/* Group 3: Routines to handle error messages.  These are organized into 
 * several subsections: 
 *  General service routines (used by more than one error reporting level)  
 *  Routines of specific error message levels
 * 
 */
/* ------------------------------------------------------------------------- */
/* Forward reference */
static void CombineSpecificCodes( int, int, int );
static const char *get_class_msg( int );

/* --BEGIN ERROR HANDLING-- */
void MPIR_Handle_fatal_error( MPIR_Comm *comm_ptr,
			      const char fcname[], int errcode )
{
    /* Define length of the the maximum error message line (or string with 
       newlines?).  This definition is used only within this routine.  */
    /* Ensure that the error message string is sufficiently long to 
       hold enough information about the error.  Use the size of the
       MPI error messages unless it is too short (defined as shown here) */
#if MPI_MAX_ERROR_STRING < 4096
#define MAX_ERRMSG_STRING 4096
#else
#define MAX_ERRMSG_STRING MPI_MAX_ERROR_STRING
#endif
    char error_msg[MAX_ERRMSG_STRING];
    int len;

    /* FIXME: Not internationalized.  Since we are using MPIR_Err_get_string,
       we are assuming that the code is still able to execute a full 
       MPICH error code to message conversion. */
    MPL_snprintf(error_msg, MAX_ERRMSG_STRING, "Fatal error in %s: ", fcname);
    len = (int)strlen(error_msg);
    MPIR_Err_get_string(errcode, &error_msg[len], MAX_ERRMSG_STRING-len, NULL);

    /* The third argument is a return code. We simply pass the error code. */
    MPID_Abort(comm_ptr, MPI_SUCCESS, errcode, error_msg);
}
/* --END ERROR HANDLING-- */

/* Check for a valid error code.  If the code is not valid, attempt to
   print out something sensible; reset the error code to have class 
   ERR_UNKNOWN */
/* FIXME: Now that error codes are chained, this does not produce a valid 
   error code since there is no valid ring index corresponding to this code */
/* FIXME: No one uses the return value */
static int checkValidErrcode( int error_class, const char fcname[], 
			      int *errcode_p )
{
    int errcode = *errcode_p;
    int rc = 0;

    if (error_class > MPICH_ERR_LAST_MPIX)
    {
	/* --BEGIN ERROR HANDLING-- */
	if (errcode & ~ERROR_CLASS_MASK)
	{
	    MPL_error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n"
			      "%s.  Please file a bug report.\n", error_class, fcname);
	    /* Note that we don't try to print the error stack; if the 
	       error code is invalid, it can't be used to find
	       the error stack.  We could consider dumping the 
	       contents of the error ring instead (without trying
	       to interpret them) */
	}
	else
	{
            /* FIXME: The error stack comment only applies to MSG_ALL */
	    MPL_error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n"
			      "%s.  Please file a bug report.  No error stack is available.\n", error_class, fcname);
	}
	/* FIXME: We probably want to set this to MPI_ERR_UNKNOWN
	   and discard the rest of the bits */
	errcode = (errcode & ~ERROR_CLASS_MASK) | MPI_ERR_UNKNOWN;
	rc = 1;
	/* --END ERROR HANDLING-- */
    }
    *errcode_p = errcode;
    return rc;
}

/* Append an error code, error2, to the end of a list of messages in the error
   ring whose head endcoded in error1_code.  An error code pointing at the
   combination is returned.  If the list of messages does not terminate cleanly
   (i.e. ring wrap has occurred), then the append is not performed. and error1
   is returned (although it may include the class of error2 if the class of
   error1 was MPI_ERR_OTHER). */
int MPIR_Err_combine_codes(int error1, int error2)
{
    int error1_code = error1;
    int error2_code = error2;
    int error2_class;
    
    /* If either error code is success, return the other */
    if (error1_code == MPI_SUCCESS) return error2_code;
    if (error2_code == MPI_SUCCESS) return error1_code;

    /* If an error code is dynamic, return that.  If both are, we choose
       error1. */
    if (error1_code & ERROR_DYN_MASK) return error1_code;
    if (error2_code & ERROR_DYN_MASK) return error2_code;

    error2_class = MPIR_ERR_GET_CLASS(error2_code);
    if (MPIR_ERR_GET_CLASS(error2_class) < MPI_SUCCESS ||
	MPIR_ERR_GET_CLASS(error2_class) > MPICH_ERR_LAST_MPIX)
    {
	error2_class = MPI_ERR_OTHER;
    }

    /* Note that this call may simply discard an error code if the error 
       message level does not support multiple codes */
    CombineSpecificCodes( error1_code, error2_code, error2_class );

    if (MPIR_ERR_GET_CLASS(error1_code) == MPI_ERR_OTHER)
    {
	error1_code = (error1_code & ~(ERROR_CLASS_MASK)) | error2_class;
    }

    return error1_code;
}

/* FIXME: This routine isn't quite right yet */
/*
 * Notes:
 * One complication is that in the instance-specific case, a ??
 */
/* 
 * Given an errorcode, place the corresponding message in msg[length].
 * The argument fn must be NULL and is otherwise ignored.
 */
void MPIR_Err_get_string( int errorcode, char * msg, int length, 
			  MPIR_Err_get_class_string_func_t fn )
{
    int error_class;
    int len, num_remaining = length;

    /* The fn (fourth) argument was added improperly and is no longer 
       used. */
    MPIR_Assert( fn == NULL );
    
    /* There was code to set num_remaining to MPI_MAX_ERROR_STRING
       if it was zero.  But based on the usage of this routine, 
       such a choice would overwrite memory. (This was caught by
       reading the coverage reports and looking into why this
       code was (thankfully!) never executed.) */
    /* if (num_remaining == 0)
       num_remaining = MPI_MAX_ERROR_STRING; */
    if (num_remaining == 0) 
	goto fn_exit;

    /* Convert the code to a string.  The cases are:
       simple class.  Find the corresponding string.
       <not done>
       if (user code) { go to code that extracts user error messages }
       else {
           is specific message code set and available?  if so, use it
	   else use generic code (lookup index in table of messages)
       }
     */
    if (errorcode & ERROR_DYN_MASK) {
	/* This is a dynamically created error code (e.g., with 
	   MPI_Err_add_class).  If a dynamic error code was created,
	   the function to convert them into strings has been set.
	   Check to see that it was; this is a safeguard against a
	   bogus error code */ 
	if (!MPIR_Process.errcode_to_string) {
	    /* FIXME: not internationalized */
	    /* --BEGIN ERROR HANDLING-- */
	    if (MPL_strncpy(msg, "Undefined dynamic error code", 
			     num_remaining))
	    {
		msg[num_remaining - 1] = '\0';
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    if (MPL_strncpy(msg, MPIR_Process.errcode_to_string( errorcode ), 
			     num_remaining))
	    {
		msg[num_remaining - 1] = '\0';
	    }
	}
    }
    else if ( (errorcode & ERROR_CLASS_MASK) == errorcode) {
	error_class = MPIR_ERR_GET_CLASS(errorcode);

        if (MPL_strncpy(msg, get_class_msg( errorcode ), num_remaining))
	    {
		msg[num_remaining - 1] = '\0';
	    }
    }
    else
    {
	/* print the class message first */
        /* FIXME: Why print the class message first? The instance
           message is supposed to be complete by itself. */
	error_class = MPIR_ERR_GET_CLASS(errorcode);

        MPL_strncpy(msg, get_class_msg(error_class), num_remaining);
	msg[num_remaining - 1] = '\0';
	len = (int)strlen(msg);
	msg += len;
	num_remaining -= len;

	/* then print the stack or the last specific error message */

        /* FIXME: Replace with function to add instance string or
           error code string */
#       if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG__ALL
	if (ErrGetInstanceString( errorcode, msg, num_remaining )) 
	    goto fn_exit;
#elif MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG__CLASS
	{
	    int generic_idx;
	    
	    generic_idx = ((errorcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;
	    
	    if (generic_idx >= 0) {
		MPL_snprintf(msg, num_remaining, ", %s", generic_err_msgs[generic_idx].long_name);
		msg[num_remaining - 1] = '\0';
		goto fn_exit;
	    }
	}
#           endif /* MSG_LEVEL >= MSG_ALL */
    }
    
fn_exit:
    return;
}

#if MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG__NONE
/* No error message support */
int MPIR_Err_create_code( int lastcode, int fatal, const char fcname[], 
			  int line, int error_class, const char generic_msg[],
			  const char specific_msg[], ... )
{
    MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND, TYPICAL, (MPL_DBG_FDEST, "%sError created: last=%#010x class=%#010x %s(%d) %s",
                                        fatal ? "Fatal " : "", lastcode, error_class, fcname, line, generic_msg));
    return (lastcode == MPI_SUCCESS) ? error_class : lastcode;
}
int MPIR_Err_create_code_valist( int lastcode, int fatal, const char fcname[], 
				 int line, int error_class, 
				 const char generic_msg[],
				 const char specific_msg[], va_list Argp )
{
    return (lastcode == MPI_SUCCESS) ? error_class : lastcode;
}
/* Internal routines */
static void CombineSpecificCodes( int error1_code, int error2_code,
				  int error2_class )
{
}
static const char *get_class_msg( int error_class )
{
    /* FIXME: Not internationalized */
    return "Error message texts are not available";
}

#elif MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG__CLASS
/* Only class error messages.  Note this is nearly the same as
   MPICH_ERROR_MSG_LEVEL == NONE, since the handling of error codes
   is the same */
int MPIR_Err_create_code( int lastcode, int fatal, const char fcname[], 
			  int line, int error_class, const char generic_msg[],
			  const char specific_msg[], ... )
{
    MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND, TYPICAL, (MPL_DBG_FDEST, "%sError created: last=%#010x class=%#010x %s(%d) %s",
                                        fatal ? "Fatal " : "", lastcode, error_class, fcname, line, generic_msg));
    return (lastcode == MPI_SUCCESS) ? error_class : lastcode;
}
int MPIR_Err_create_code_valist( int lastcode, int fatal, const char fcname[], 
				 int line, int error_class, 
				 const char generic_msg[],
				 const char specific_msg[], va_list Argp )
{
    return (lastcode == MPI_SUCCESS) ? error_class : lastcode;
}

static void CombineSpecificCodes( int error1_code, int error2_code,
				  int error2_class )
{
}
static const char *get_class_msg( int error_class )
{
    if (error_class >= 0 && error_class < MPIR_MAX_ERROR_CLASS_INDEX) {
	return classToMsg[error_class];
    }
    else {
	/* --BEGIN ERROR HANDLING-- */
	return "Unknown error class";
	/* --END ERROR HANDLING-- */
    }
}

#elif MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG__GENERIC
#define NEEDS_FIND_GENERIC_MSG_INDEX
static int FindGenericMsgIndex( const char [] );

/* Only generic error messages (more than class, but all predefined) */

int MPIR_Err_create_code( int lastcode, int fatal, const char fcname[], 
			  int line, int error_class, const char generic_msg[],
			  const char specific_msg[], ... )
{
    int rc;
    va_list Argp;
    va_start(Argp, specific_msg);
    MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND, TYPICAL, (MPL_DBG_FDEST, "%sError created: last=%#010x class=%#010x %s(%d) %s",
                                        fatal ? "Fatal " : "", lastcode, error_class, fcname, line, generic_msg));
    rc = MPIR_Err_create_code_valist( lastcode, fatal, fcname, line,
				      error_class, generic_msg, specific_msg,
				      Argp );
    va_end(Argp);
    return rc;
}
int MPIR_Err_create_code_valist( int lastcode, int fatal, const char fcname[], 
				 int line, int error_class, 
				 const char generic_msg[],
				 const char specific_msg[], va_list Argp )
{
    int generic_idx;
    int errcode = lastcode;
    if (lastcode == MPI_SUCCESS) {
	generic_idx = FindGenericMsgIndex(generic_msg);
	if (generic_idx >= 0) {
	    errcode = (generic_idx << ERROR_GENERIC_SHIFT) | error_class;
	    if (fatal)
		errcode |= ERROR_FATAL_MASK;
	}
    }
    return errcode;
}

static void CombineSpecificCodes( int error1_code, int error2_code,
				  int error2_class )
{
}
static const char *get_class_msg( int error_class )
{
    if (error_class >= 0 && error_class < MPIR_MAX_ERROR_CLASS_INDEX) {
	return generic_err_msgs[class_to_index[error_class]].long_name;
    }
    else {
	/* --BEGIN ERROR HANDLING-- */
	return "Unknown error class";
	/* --END ERROR HANDLING-- */
    }
}

#elif MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG__ALL
/* General error message support, including the error message stack */

static int checkErrcodeIsValid( int );
static const char *ErrcodeInvalidReasonStr( int );
#define NEEDS_FIND_GENERIC_MSG_INDEX
static int FindGenericMsgIndex( const char [] );
static int FindSpecificMsgIndex( const char [] );
static int vsnprintf_mpi(char *str, size_t maxlen, const char *fmt_orig, 
			 va_list list);
static void ErrcodeCreateID( int error_class, int generic_idx, 
			     const char *msg, int *id, int *seq );
static int convertErrcodeToIndexes( int errcode, int *ring_idx, int *ring_id,
				    int *generic_idx );
static void MPIR_Err_print_stack_string(int errcode, char *str, int maxlen);

#define MAX_ERROR_RING ERROR_SPECIFIC_INDEX_SIZE
#define MAX_LOCATION_LEN 63

/* The maximum error string in this case may be a multi-line message,
   constructed from multiple entries in the error message ring.  The 
   individual ring messages should be shorter than MPI_MAX_ERROR_STRING,
   perhaps as small as 256. We define a separate value for the error lines. 
 */
#define MPIR_MAX_ERROR_LINE 256

/* See the description above for the fields in this structure */
typedef struct MPIR_Err_msg
{
    int  id;
    int  prev_error;
    int  use_user_error_code;
    int  user_error_code;

    char location[MAX_LOCATION_LEN+1];
    char msg[MPIR_MAX_ERROR_LINE+1];
}
MPIR_Err_msg_t;

static MPIR_Err_msg_t ErrorRing[MAX_ERROR_RING];
static volatile unsigned int error_ring_loc     = 0;
static volatile unsigned int max_error_ring_loc = 0;

/* FIXME: This needs to be made consistent with the different thread levels, 
   since in the "global" thread level, an extra thread mutex is not required. */
#if defined(MPID_REQUIRES_THREAD_SAFETY)
/* if the device requires internal MPICH routines to be thread safe, the
   MPID_THREAD_CHECK macros are not appropriate */
static MPID_Thread_mutex_t error_ring_mutex;
#define error_ring_mutex_create(_mpi_errno_p_)			\
    MPID_Thread_mutex_create(&error_ring_mutex, _mpi_errno_p_)
#define error_ring_mutex_destroy(_mpi_errno_p)				\
    MPID_Thread_mutex_destroy(&error_ring_mutex, _mpi_errno_p_)
#define error_ring_mutex_lock()                                 \
    do {                                                        \
        int err;                                                \
        MPID_Thread_mutex_lock(&error_ring_mutex, &err);        \
    } while (0)
#define error_ring_mutex_unlock()                               \
    do {                                                        \
        int err;                                                \
        MPID_Thread_mutex_unlock(&error_ring_mutex, &err);      \
    } while (0)
#elif defined(MPICH_IS_THREADED)
static MPID_Thread_mutex_t error_ring_mutex;
#define error_ring_mutex_create(_mpi_errno_p) MPID_Thread_mutex_create(&error_ring_mutex,_mpi_errno_p)
#define error_ring_mutex_destroy(_mpi_errno_p) MPID_Thread_mutex_destroy(&error_ring_mutex,_mpi_errno_p)
#define error_ring_mutex_lock()                          \
    do {                                                 \
        int err;                                         \
        if (did_err_init) {                              \
            MPIR_THREAD_CHECK_BEGIN;                        \
            MPID_Thread_mutex_lock(&error_ring_mutex,&err); \
            MPIR_THREAD_CHECK_END;                          \
        }                                                \
    } while (0)
#define error_ring_mutex_unlock()                        \
    do {                                                 \
        int err;                                         \
        if (did_err_init) {                              \
            MPIR_THREAD_CHECK_BEGIN;                          \
            MPID_Thread_mutex_unlock(&error_ring_mutex,&err); \
            MPIR_THREAD_CHECK_END;                            \
        }                                                \
    } while (0)
#else
#define error_ring_mutex_create(_a)
#define error_ring_mutex_destroy(_a)
#define error_ring_mutex_lock()
#define error_ring_mutex_unlock()
#endif /* REQUIRES_THREAD_SAFETY */


int MPIR_Err_create_code( int lastcode, int fatal, const char fcname[], 
			  int line, int error_class, const char generic_msg[],
			  const char specific_msg[], ... )
{
    int rc;
    va_list Argp;
    va_start(Argp, specific_msg);
    MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND, TYPICAL, (MPL_DBG_FDEST, "%sError created: last=%#010x class=%#010x %s(%d) %s",
                                        fatal ? "Fatal " : "", lastcode, error_class, fcname, line, generic_msg));
    rc = MPIR_Err_create_code_valist( lastcode, fatal, fcname, line,
				      error_class, generic_msg, specific_msg,
				      Argp );
    va_end(Argp);
    return rc;
}
/*
 * This is the real routine for generating an error code.  It takes
 * a va_list so that it can be called by any routine that accepts a 
 * variable number of arguments.
 */
int MPIR_Err_create_code_valist( int lastcode, int fatal, const char fcname[], 
				 int line, int error_class, 
				 const char generic_msg[],
				 const char specific_msg[], va_list Argp )
{
    int err_code;
    int generic_idx;
    int use_user_error_code = 0;
    int user_error_code = -1;
    char user_ring_msg[MPIR_MAX_ERROR_LINE+1];

    /* Create the code from the class and the message ring index */

    /* Check that lastcode is valid */
    if (lastcode != MPI_SUCCESS) {
	int reason;
	reason = checkErrcodeIsValid(lastcode);
	if (reason) {
	    /* --BEGIN ERROR HANDLING-- */
	    MPL_error_printf( "INTERNAL ERROR: invalid error code %x (%s) in %s:%d\n", 
			       lastcode, ErrcodeInvalidReasonStr( reason ), 
			       fcname, line );
	    lastcode = MPI_SUCCESS;
	    /* --END ERROR HANDLING-- */
	}
    }

    /* FIXME: ERR_OTHER is overloaded; this may mean "OTHER" or it may
       mean "No additional error, just routine stack info" */
    if (error_class == MPI_ERR_OTHER)
    {
        if (MPIR_ERR_GET_CLASS(lastcode) > MPI_SUCCESS && 
	    MPIR_ERR_GET_CLASS(lastcode) <= MPICH_ERR_LAST_MPIX)
	{
	    /* If the last class is more specific (and is valid), then pass it 
	       through */
	    error_class = MPIR_ERR_GET_CLASS(lastcode);
	}
	else
	{
	    error_class = MPI_ERR_OTHER;
	}
    }

    /* Handle special case of MPI_ERR_IN_STATUS.  According to the standard,
       the code must be equal to the class. See section 3.7.5.  
       Information on the particular error is in the MPI_ERROR field 
       of the status. */
    if (error_class == MPI_ERR_IN_STATUS)
    {
	return MPI_ERR_IN_STATUS;
    }

    err_code = error_class;

    /* Handle the generic message.  This selects a subclass, based on a text 
       string */
    generic_idx = FindGenericMsgIndex(generic_msg);
    if (generic_idx >= 0) {
	if (strcmp( generic_err_msgs[generic_idx].short_name, "**user" ) == 0) {
	    use_user_error_code = 1;
	    /* This is a special case.  The format is
	       "**user", "**userxxx %d", intval
	       (generic, specific, parameter).  In this
	       case we must ... save the user value because
	       we store it explicitly in the ring.  
	       We do this here because we cannot both access the 
	       user error code and pass the argp to vsnprintf_mpi . */
	    if (specific_msg) {
		const char *specific_fmt; 
		int specific_idx;
		user_error_code = va_arg(Argp,int);
		specific_idx = FindSpecificMsgIndex(specific_msg);
		if (specific_idx >= 0) {
		    specific_fmt = specific_err_msgs[specific_idx].long_name;
		}
		else {
		    specific_fmt = specific_msg;
		}
		MPL_snprintf( user_ring_msg, sizeof(user_ring_msg),
			       specific_fmt, user_error_code );
	    }
	    else {
		user_ring_msg[0] = 0;
	    }
	}
	err_code |= (generic_idx + 1) << ERROR_GENERIC_SHIFT;
    }
    else {
	/* TODO: lookup index for class error message */
	err_code &= ~ERROR_GENERIC_MASK;
	
#           ifdef MPICH_DBG_OUTPUT
	{
	    if (generic_msg[0] == '*' && generic_msg[1] == '*')
		{
                    MPL_error_printf( "INTERNAL ERROR: Could not find %s in list of messages\n", generic_msg );
		}
	}
#           endif /* DBG_OUTPUT */
    }

    /* Handle the instance-specific part of the error message */
    {
	int specific_idx;
	const char * specific_fmt = 0;
	int  ring_idx, ring_seq=0;
	char * ring_msg;
	
	error_ring_mutex_lock();
	{
	    /* Get the next entry in the ring; keep track of what part of the 
	       ring is in use (max_error_ring_loc) */
	    ring_idx = error_ring_loc++;
	    if (error_ring_loc >= MAX_ERROR_RING) 
		error_ring_loc %= MAX_ERROR_RING;
	    if (error_ring_loc > max_error_ring_loc)
		max_error_ring_loc = error_ring_loc;
	
	    ring_msg = ErrorRing[ring_idx].msg;

	    if (specific_msg != NULL)
	    {
		specific_idx = FindSpecificMsgIndex(specific_msg);
		if (specific_idx >= 0)
		{
		    specific_fmt = specific_err_msgs[specific_idx].long_name;
		}
		else
		{
		    specific_fmt = specific_msg;
		}
		/* See the code above for handling user errors */
		if (!use_user_error_code) {
		    vsnprintf_mpi( ring_msg, MPIR_MAX_ERROR_LINE, 
				   specific_fmt, Argp );
		}
		else {
		    MPL_strncpy( ring_msg, user_ring_msg, MPIR_MAX_ERROR_LINE );
		}
	    }
	    else if (generic_idx >= 0)
	    {
		MPL_strncpy( ring_msg,generic_err_msgs[generic_idx].long_name,
			      MPIR_MAX_ERROR_LINE );
	    }
	    else
	    {
		MPL_strncpy( ring_msg, generic_msg, MPIR_MAX_ERROR_LINE );
	    }

	    ring_msg[MPIR_MAX_ERROR_LINE] = '\0';
	
	    /* Get the ring sequence number and set the ring id */
	    ErrcodeCreateID( error_class, generic_idx, ring_msg, 
			     &ErrorRing[ring_idx].id, &ring_seq );
	    /* Set the previous code. */
	    ErrorRing[ring_idx].prev_error = lastcode;

	    /* */
	    if (use_user_error_code)
	    {
		ErrorRing[ring_idx].use_user_error_code = 1;
		ErrorRing[ring_idx].user_error_code     = user_error_code;
	    }
	    else if (lastcode != MPI_SUCCESS)
	    {
		int last_ring_idx;
		int last_ring_id;
		int last_generic_idx;

		if (convertErrcodeToIndexes( lastcode, &last_ring_idx, 
					     &last_ring_id,
					     &last_generic_idx ) != 0) {
		    /* --BEGIN ERROR HANDLING-- */
		    MPL_error_printf( 
		  "Invalid error code (%d) (error ring index %d invalid)\n", 
		  lastcode, last_ring_idx );
		    /* --END ERROR HANDLING-- */
		}
		else {
		    if (last_generic_idx >= 0 && 
			ErrorRing[last_ring_idx].id == last_ring_id) {
			if (ErrorRing[last_ring_idx].use_user_error_code) {
			    ErrorRing[ring_idx].use_user_error_code = 1;
			    ErrorRing[ring_idx].user_error_code = 
				ErrorRing[last_ring_idx].user_error_code;
			}
		    }
		}
	    }

	    if (fcname != NULL)
	    {
		MPL_snprintf(ErrorRing[ring_idx].location, MAX_LOCATION_LEN, "%s(%d)", fcname, line);
		ErrorRing[ring_idx].location[MAX_LOCATION_LEN] = '\0';
	    }
	    else
	    {
		ErrorRing[ring_idx].location[0] = '\0';
	    }
            {
                MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND,VERBOSE,(MPL_DBG_FDEST, "New ErrorRing[%d]", ring_idx));
                MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND,VERBOSE,(MPL_DBG_FDEST, "    id         = %#010x", ErrorRing[ring_idx].id));
                MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND,VERBOSE,(MPL_DBG_FDEST, "    prev_error = %#010x", ErrorRing[ring_idx].prev_error));
                MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND,VERBOSE,(MPL_DBG_FDEST, "    user=%d", ErrorRing[ring_idx].use_user_error_code));
            }
	}
	error_ring_mutex_unlock();

	err_code |= ring_idx << ERROR_SPECIFIC_INDEX_SHIFT;
	err_code |= ring_seq << ERROR_SPECIFIC_SEQ_SHIFT;

    }

    if (fatal || MPIR_Err_is_fatal(lastcode))
    {
	err_code |= ERROR_FATAL_MASK;
    }
    
    return err_code;
}

/* FIXME: Shouldn't str be const char * ? - no, but you don't know that without
   some documentation */
static void MPIR_Err_print_stack_string(int errcode, char *str, int maxlen )
{
    char *str_orig = str;
    int len;

    error_ring_mutex_lock();
    {
	/* Find the longest fcname in the stack */
	int max_location_len = 0;
	int tmp_errcode = errcode;
	while (tmp_errcode != MPI_SUCCESS) {
	    int ring_idx;
	    int ring_id;
	    int generic_idx;
	    
	    if (convertErrcodeToIndexes( tmp_errcode, &ring_idx, &ring_id,
					 &generic_idx ) != 0) {
		/* --BEGIN ERROR HANDLING-- */
		MPL_error_printf( 
		    "Invalid error code (%d) (error ring index %d invalid)\n", 
		    errcode, ring_idx );
		break;
		/* --END ERROR HANDLING-- */
	    }

	    if (generic_idx < 0) {
		break;
	    }
	    
	    if (ErrorRing[ring_idx].id == ring_id) {
		len = (int)strlen(ErrorRing[ring_idx].location);
		max_location_len = MPL_MAX(max_location_len, len);
		tmp_errcode = ErrorRing[ring_idx].prev_error;
	    }
	    else
	    {
		break;
	    }
	}
	max_location_len += 2; /* add space for the ": " */
	/* print the error stack */
	while (errcode != MPI_SUCCESS) {
	    int ring_idx;
	    int ring_id;
	    int generic_idx;
	    int i;
	    char *cur_pos;
	    
	    if (convertErrcodeToIndexes( errcode, &ring_idx, &ring_id,
					 &generic_idx ) != 0) {
		/* --BEGIN ERROR HANDLING-- */
		MPL_error_printf( 
		    "Invalid error code (%d) (error ring index %d invalid)\n", 
		    errcode, ring_idx );
		/* --END ERROR HANDLING-- */
	    }
	    
	    if (generic_idx < 0)
	    {
		break;
	    }
	    
	    if (ErrorRing[ring_idx].id == ring_id) {
		int nchrs;
		MPL_snprintf(str, maxlen, "%s", ErrorRing[ring_idx].location);
		len     = (int)strlen(str);
		maxlen -= len;
		str    += len;
		nchrs   = max_location_len - 
		    (int)strlen(ErrorRing[ring_idx].location) - 2;
		while (nchrs > 0 && maxlen > 0) {
		    *str++ = '.';
		    nchrs--;
		    maxlen--;
		}
		if (maxlen > 0) {
		    *str++ = ':';
		    maxlen--;
		}
		if (maxlen > 0) {
		    *str++ = ' ';
		    maxlen--;
		}
		
		if (MPIR_CVAR_CHOP_ERROR_STACK > 0)
		{
		    cur_pos = ErrorRing[ring_idx].msg;
		    len = (int)strlen(cur_pos);
		    if (len == 0 && maxlen > 0) {
			*str++ = '\n';
			maxlen--;
		    }
		    while (len)
		    {
			if (len >= MPIR_CVAR_CHOP_ERROR_STACK - max_location_len)
			{
			    if (len > maxlen)
				break;
			    /* FIXME: Don't use Snprint to append a string ! */
			    MPL_snprintf(str, MPIR_CVAR_CHOP_ERROR_STACK - 1 - max_location_len, "%s", cur_pos);
			    str[MPIR_CVAR_CHOP_ERROR_STACK - 1 - max_location_len] = '\n';
			    cur_pos += MPIR_CVAR_CHOP_ERROR_STACK - 1 - max_location_len;
			    str += MPIR_CVAR_CHOP_ERROR_STACK - max_location_len;
			    maxlen -= MPIR_CVAR_CHOP_ERROR_STACK - max_location_len;
			    if (maxlen < max_location_len)
				break;
			    for (i=0; i<max_location_len; i++)
			    {
				MPL_snprintf(str, maxlen, " ");
				maxlen--;
				str++;
			    }
			    len = (int)strlen(cur_pos);
			}
			else
			{
			    MPL_snprintf(str, maxlen, "%s\n", cur_pos);
			    len = (int)strlen(str);
			    maxlen -= len;
			    str += len;
			    len = 0;
			}
		    }
		}
		else
		{
		    MPL_snprintf(str, maxlen, "%s\n", ErrorRing[ring_idx].msg);
		    len = (int)strlen(str);
		    maxlen -= len;
		    str += len;
		}
		errcode = ErrorRing[ring_idx].prev_error;
	    }
	    else
	    {
		break;
	    }
	}
    }
    error_ring_mutex_unlock();

    if (errcode == MPI_SUCCESS)
    {
	goto fn_exit;
    }

    /* FIXME: The following code is broken as described above (if the errcode
       is not valid, then this code is just going to cause more problems) */
    {
	int generic_idx;
	
	generic_idx = ((errcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;
	
	if (generic_idx >= 0)
	{
	    const char *p;
	    /* FIXME: (Here and elsewhere)  Make sure any string is
	       non-null before you use it */
	    p = generic_err_msgs[generic_idx].long_name;
	    if (!p) { p = "<NULL>"; }
	    MPL_snprintf(str, maxlen, "(unknown)(): %s\n", p );
	    len = (int)strlen(str);
	    maxlen -= len;
	    str += len;
	    goto fn_exit;
	}
    }
    
    {
	int error_class;

	error_class = ERROR_GET_CLASS(errcode);
	
	if (error_class <= MPICH_ERR_LAST_MPIX)
	{
	    MPL_snprintf(str, maxlen, "(unknown)(): %s\n", 
			  get_class_msg(ERROR_GET_CLASS(errcode)));
	    len = (int)strlen(str);
	    maxlen -= len;
	    str += len;
	}
	else
	{
	    /* FIXME: Not internationalized */
	    MPL_snprintf(str, maxlen, 
			  "Error code contains an invalid class (%d)\n",
			  error_class);
	    len = (int)strlen(str);
	    maxlen -= len;
	    str += len;
	}
    }
    
 fn_exit:
    if (str_orig != str)
    {
	str--;
	*str = '\0';
    }
    return;
}


/* Internal Routines */

static const char *get_class_msg( int error_class )
{
    if (error_class >= 0 && error_class < MPIR_MAX_ERROR_CLASS_INDEX) {
	return generic_err_msgs[class_to_index[error_class]].long_name;
    }
    else {
	/* --BEGIN ERROR HANDLING-- */
	return "Unknown error class";
	/* --END ERROR HANDLING-- */
    }
}

/* 
 * Given a message string abbreviation (e.g., one that starts "**"), return 
 * the corresponding index.  For the specific
 * (parameterized messages), use idx = FindSpecificMsgIndex( "**msg" );
 * Note: Identical to FindGeneric, but with a different array.  Should
 * use a single routine.
 */
static int FindSpecificMsgIndex( const char msg[] )
{
    int i, c;
    for (i=0; i<specific_msgs_len; i++) {
	/* Check the sentinals to insure that the values are ok first */
	if (specific_err_msgs[i].sentinal1 != 0xacebad03 ||
	    specific_err_msgs[i].sentinal2 != 0xcb0bfa11) {
	    /* Something bad has happened! Don't risk trying the
	       short_name pointer; it may have been corrupted */
	    break;
	}
	c = strcmp( specific_err_msgs[i].short_name, msg );
	if (c == 0) return i;
	if (c > 0)
	{
	    /* don't return here if the string partially matches */
	    if (strncmp(specific_err_msgs[i].short_name, msg, strlen(msg)) != 0)
		return -1;
	}
    }
    return -1;
}
/* See FindGenericMsgIndex comments for a more efficient search routine that
   could be used here as well. */

/* Support for the instance-specific messages */
/* ------------------------------------------------------------------------- */
/* Routines to convert instance-specific messages into a string              */
/* This is the only case that supports instance-specific messages            */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------ */
/* This block of code is used to convert various MPI values into descriptive*/
/* strings.  The routines are                                               */
/*     GetAssertString - handle MPI_MODE_xxx (RMA asserts)                  */
/*     GetDTypeString  - handle MPI_Datatypes                               */
/*     GetMPIOpString  - handle MPI_Op                                      */
/* These routines are used in vsnprintf_mpi                                 */
/* FIXME: These functions are not thread safe                               */
/* ------------------------------------------------------------------------ */
#define ASSERT_STR_MAXLEN 256

static const char * GetAssertString(int d)
{
    static char str[ASSERT_STR_MAXLEN] = "";
    char *cur;
    size_t len = ASSERT_STR_MAXLEN;
    size_t n;

    if (d == 0)
    {
	MPL_strncpy(str, "assert=0", ASSERT_STR_MAXLEN);
	return str;
    }
    cur = str;
    if (d & MPI_MODE_NOSTORE)
    {
	MPL_strncpy(cur, "MPI_MODE_NOSTORE", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOSTORE;
    }
    if (d & MPI_MODE_NOCHECK)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPL_strncpy(cur, " | MPI_MODE_NOCHECK", len);
	else
	    MPL_strncpy(cur, "MPI_MODE_NOCHECK", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOCHECK;
    }
    if (d & MPI_MODE_NOPUT)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPL_strncpy(cur, " | MPI_MODE_NOPUT", len);
	else
	    MPL_strncpy(cur, "MPI_MODE_NOPUT", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOPUT;
    }
    if (d & MPI_MODE_NOPRECEDE)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPL_strncpy(cur, " | MPI_MODE_NOPRECEDE", len);
	else
	    MPL_strncpy(cur, "MPI_MODE_NOPRECEDE", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOPRECEDE;
    }
    if (d & MPI_MODE_NOSUCCEED)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPL_strncpy(cur, " | MPI_MODE_NOSUCCEED", len);
	else
	    MPL_strncpy(cur, "MPI_MODE_NOSUCCEED", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOSUCCEED;
    }
    if (d)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPL_snprintf(cur, len, " | 0x%x", d);
	else
	    MPL_snprintf(cur, len, "assert=0x%x", d);
    }
    return str;
}

static const char * GetDTypeString(MPI_Datatype d)
{
    static char default_str[64];
    int num_integers, num_addresses, num_datatypes, combiner = 0;
    char *str;

    if (HANDLE_GET_MPI_KIND(d) != MPIR_DATATYPE ||      \
	(HANDLE_GET_KIND(d) == HANDLE_KIND_INVALID &&   \
	d != MPI_DATATYPE_NULL))
        return "INVALID DATATYPE";
    

    if (d == MPI_DATATYPE_NULL)
	return "MPI_DATATYPE_NULL";

    if (d == 0)
    {
	MPL_strncpy(default_str, "dtype=0x0", sizeof(default_str));
	return default_str;
    }

    MPIR_Type_get_envelope(d, &num_integers, &num_addresses, &num_datatypes, 
			   &combiner);
    if (combiner == MPI_COMBINER_NAMED)
    {
	str = MPIR_Datatype_builtin_to_string(d);
	if (str == NULL)
	{
	    MPL_snprintf(default_str, sizeof(default_str), "dtype=0x%08x", d);
	    return default_str;
	}
	return str;
    }
    
    /* default is not thread safe */
    str = MPIR_Datatype_combiner_to_string(combiner);
    if (str == NULL)
    {
	MPL_snprintf(default_str, sizeof(default_str), "dtype=USER<0x%08x>", d);
	return default_str;
    }
    MPL_snprintf(default_str, sizeof(default_str), "dtype=USER<%s>", str);
    return default_str;
}

static const char * GetMPIOpString(MPI_Op o)
{
    static char default_str[64];

    switch (o)
    {
    case MPI_OP_NULL:
	return "MPI_OP_NULL";
    case MPI_MAX:
	return "MPI_MAX";
    case MPI_MIN:
	return "MPI_MIN";
    case MPI_SUM:
	return "MPI_SUM";
    case MPI_PROD:
	return "MPI_PROD";
    case MPI_LAND:
	return "MPI_LAND";
    case MPI_BAND:
	return "MPI_BAND";
    case MPI_LOR:
	return "MPI_LOR";
    case MPI_BOR:
	return "MPI_BOR";
    case MPI_LXOR:
	return "MPI_LXOR";
    case MPI_BXOR:
	return "MPI_BXOR";
    case MPI_MINLOC:
	return "MPI_MINLOC";
    case MPI_MAXLOC:
	return "MPI_MAXLOC";
    case MPI_REPLACE:
	return "MPI_REPLACE";
    case MPI_NO_OP:
        return "MPI_NO_OP";
    }
    /* FIXME: default is not thread safe */
    MPL_snprintf(default_str, sizeof(default_str), "op=0x%x", o);
    return default_str;
}

/* ------------------------------------------------------------------------ */
/* This routine takes an instance-specific string with format specifiers    */
/* This routine makes use of the above routines, along with some inlined    */
/* code, to process the format specifiers for the MPI objects               */
/* The current set of format specifiers is undocumented except for their
   use in this routine.  In addition, these choices do not permit the 
   use of GNU extensions to check the validity of these arguments.  
   At some point, a documented set that can exploit those GNU extensions
   will replace these. */
/* ------------------------------------------------------------------------ */

static int vsnprintf_mpi(char *str, size_t maxlen, const char *fmt_orig, 
			 va_list list)
{
    char *begin, *end, *fmt;
    size_t len;
    MPI_Comm C;
    MPI_Info info;
    MPI_Datatype D;
    MPI_Win W;
    MPI_Group G;
    MPI_Op O;
    MPI_Request R;
    MPI_Errhandler E;
    char *s;
    int t, i, d, mpi_errno=MPI_SUCCESS;
    long long ll;
    MPI_Count c;
    void *p;

    fmt = MPL_strdup(fmt_orig);
    if (fmt == NULL)
    {
	if (maxlen > 0 && str != NULL)
	    *str = '\0';
	return 0;
    }

    begin = fmt;
    end = strchr(fmt, '%');
    while (end)
    {
	len = maxlen;
	if (len > (size_t)(end - begin)) {
	    len = (size_t)(end - begin);
	}
	if (len)
	{
	    MPIR_Memcpy(str, begin, len);
	    str += len;
	    maxlen -= len;
	}
	end++;
	begin = end+1;
	switch ((int)(*end))
	{
	case (int)'s':
	    s = va_arg(list, char *);
	    if (s) 
	        MPL_strncpy(str, s, maxlen);
            else {
		MPL_strncpy(str, "<NULL>", maxlen );
	    }
	    break;
	case (int)'d':
	    d = va_arg(list, int);
	    MPL_snprintf(str, maxlen, "%d", d);
	    break;
	case (int)'L':
	    ll = va_arg(list, long long);
	    MPL_snprintf(str, maxlen, "%lld", ll);
	    break;
        case (int)'x':
            d = va_arg(list, int);
            MPL_snprintf(str, maxlen, "%x", d);
            break;
        case (int)'X':
            ll = va_arg(list, long long);
            MPL_snprintf(str, maxlen, "%llx", ll);
            break;
	case (int)'i':
	    i = va_arg(list, int);
	    switch (i)
	    {
	    case MPI_ANY_SOURCE:
		MPL_strncpy(str, "MPI_ANY_SOURCE", maxlen);
		break;
	    case MPI_PROC_NULL:
		MPL_strncpy(str, "MPI_PROC_NULL", maxlen);
		break;
	    case MPI_ROOT:
		MPL_strncpy(str, "MPI_ROOT", maxlen);
		break;
	    default:
		MPL_snprintf(str, maxlen, "%d", i);
		break;
	    }
	    break;
	case (int)'t':
	    t = va_arg(list, int);
	    switch (t)
	    {
	    case MPI_ANY_TAG:
		MPL_strncpy(str, "MPI_ANY_TAG", maxlen);
		break;
	    default:
                /* Note that MPI_UNDEFINED is not a valid tag value,
                   though there is one example in the MPI-3.0 standard
                   that sets status.MPI_TAG to MPI_UNDEFINED in a 
                   generalized request example. */
		MPL_snprintf(str, maxlen, "%d", t);
		break;
	    }
	    break;
	case (int)'p':
	    p = va_arg(list, void *);
	    /* FIXME: A check for MPI_IN_PLACE should only be used 
	       where that is valid */
	    if (p == MPI_IN_PLACE)
	    {
		MPL_strncpy(str, "MPI_IN_PLACE", maxlen);
	    }
	    else
	    {
		/* FIXME: We may want to use 0x%p for systems that 
		   (including Windows) that don't prefix %p with 0x. 
		   This must be done with a capability, not a test on
		   particular OS or header files */
		MPL_snprintf(str, maxlen, "%p", p);
	    }
	    break;
	case (int)'C':
	    C = va_arg(list, MPI_Comm);
	    switch (C)
	    {
	    case MPI_COMM_WORLD:
		MPL_strncpy(str, "MPI_COMM_WORLD", maxlen);
		break;
	    case MPI_COMM_SELF:
		MPL_strncpy(str, "MPI_COMM_SELF", maxlen);
		break;
	    case MPI_COMM_NULL:
		MPL_strncpy(str, "MPI_COMM_NULL", maxlen);
		break;
	    default:
		MPL_snprintf(str, maxlen, "comm=0x%x", C);
		break;
	    }
	    break;
	case (int)'I':
	    info = va_arg(list, MPI_Info);
	    if (info == MPI_INFO_NULL)
	    {
		MPL_strncpy(str, "MPI_INFO_NULL", maxlen);
	    }
	    else
	    {
		MPL_snprintf(str, maxlen, "info=0x%x", info);
	    }
	    break;
	case (int)'D':
	    D = va_arg(list, MPI_Datatype);
	    MPL_snprintf(str, maxlen, "%s", GetDTypeString(D));
	    break;
	    /* Include support for %F only if MPI-IO is enabled */
#ifdef MPI_MODE_RDWR
	case (int)'F':
	    {
	    MPI_File F;
	    F = va_arg(list, MPI_File);
	    if (F == MPI_FILE_NULL)
	    {
		MPL_strncpy(str, "MPI_FILE_NULL", maxlen);
	    }
	    else
	    {
		MPL_snprintf(str, maxlen, "file=0x%lx", (unsigned long)F);
	    }
	    }
	    break;
#endif /* MODE_RDWR */
	case (int)'W':
	    W = va_arg(list, MPI_Win);
	    if (W == MPI_WIN_NULL)
	    {
		MPL_strncpy(str, "MPI_WIN_NULL", maxlen);
	    }
	    else
	    {
		MPL_snprintf(str, maxlen, "win=0x%x", W);
	    }
	    break;
	case (int)'A':
	    d = va_arg(list, int);
	    MPL_snprintf(str, maxlen, "%s", GetAssertString(d));
	    break;
	case (int)'G':
	    G = va_arg(list, MPI_Group);
	    if (G == MPI_GROUP_NULL)
	    {
		MPL_strncpy(str, "MPI_GROUP_NULL", maxlen);
	    }
	    else
	    {
		MPL_snprintf(str, maxlen, "group=0x%x", G);
	    }
	    break;
	case (int)'O':
	    O = va_arg(list, MPI_Op);
	    MPL_snprintf(str, maxlen, "%s", GetMPIOpString(O));
	    break;
	case (int)'R':
	    R = va_arg(list, MPI_Request);
	    if (R == MPI_REQUEST_NULL)
	    {
		MPL_strncpy(str, "MPI_REQUEST_NULL", maxlen);
	    }
	    else
	    {
		MPL_snprintf(str, maxlen, "req=0x%x", R);
	    }
	    break;
	case (int)'E':
	    E = va_arg(list, MPI_Errhandler);
	    if (E == MPI_ERRHANDLER_NULL)
	    {
		MPL_strncpy(str, "MPI_ERRHANDLER_NULL", maxlen);
	    }
	    else
	    {
		MPL_snprintf(str, maxlen, "errh=0x%x", E);
	    }
	    break;
        case (int)'c':
            c = va_arg(list, MPI_Count);
            MPIR_Assert(sizeof(long long) >= sizeof(MPI_Count));
            MPL_snprintf(str, maxlen, "%lld", (long long)c);
            break;
	default:
	    /* Error: unhandled output type */
	    return 0;
	    /*
	    if (maxlen > 0 && str != NULL)
		*str = '\0';
	    break;
	    */
	}
	len = strlen(str);
	maxlen -= len;
	str += len;
	end = strchr(begin, '%');
    }
    if (*begin != '\0')
    {
	MPL_strncpy(str, begin, maxlen);
    }
    /* Free the dup'ed format string */
    MPL_free( fmt );

    return mpi_errno;
}

/* ------------------------------------------------------------------------- */
/* Manage the error reporting stack                                          */
/* ------------------------------------------------------------------------- */

/* 
 * Support for multiple messages, including the error message ring.
 * In principle, the error message ring could use used to provide
 * support for multiple error classes or codes, without providing
 * instance-specific support.  However, for now, we combine the two
 * capabilities.
 */


static void MPIR_Err_stack_init( void )
{
    int mpi_errno = MPI_SUCCESS;

    error_ring_mutex_create(&mpi_errno);

    if (MPIR_CVAR_CHOP_ERROR_STACK < 0) {
        MPIR_CVAR_CHOP_ERROR_STACK = 80;
#ifdef HAVE_WINDOWS_H
        {
            /* If windows, set the default width to the window size */
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hConsole != INVALID_HANDLE_VALUE)
            {
                CONSOLE_SCREEN_BUFFER_INFO info;
                if (GetConsoleScreenBufferInfo(hConsole, &info))
                {
                    /* override the parameter system in this case */
                    MPIR_CVAR_CHOP_ERROR_STACK = info.dwMaximumWindowSize.X;
                }
            }
        }
#endif /* WINDOWS_H */
    }
}

/* Create the ring id from information about the message */
static void ErrcodeCreateID( int error_class, int generic_idx, 
			     const char *msg, int *id, int *seq )
{
    int i;
    int ring_seq = 0, ring_id;

    /* Create a simple hash function of the message to serve as the 
       sequence number */
    ring_seq = 0;
    for (i=0; msg[i]; i++)
	ring_seq += (unsigned int) msg[i];

    ring_seq %= ERROR_SPECIFIC_SEQ_SIZE;

    ring_id = (error_class & ERROR_CLASS_MASK) |
	((generic_idx + 1) << ERROR_GENERIC_SHIFT) |
	(ring_seq << ERROR_SPECIFIC_SEQ_SHIFT);

    *id  = ring_id;
    *seq = ring_seq;
}

/* Convert an error code into ring_idx, ring_id, and generic_idx.
   Return non-zero if there is a problem with the decode values
   (e.g., out of range for the ring index) */
static int convertErrcodeToIndexes( int errcode, int *ring_idx, int *ring_id,
				    int *generic_idx )
{
    *ring_idx = (errcode & ERROR_SPECIFIC_INDEX_MASK) >> 
	ERROR_SPECIFIC_INDEX_SHIFT;
    *ring_id = errcode & (ERROR_CLASS_MASK | 
			  ERROR_GENERIC_MASK | ERROR_SPECIFIC_SEQ_MASK);
    *generic_idx = ((errcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;
    
    /* Test on both the max_error_ring_loc and MAX_ERROR_RING to guard 
       against memory overwrites */
    if (*ring_idx < 0 || *ring_idx >= MAX_ERROR_RING || 
	*ring_idx > max_error_ring_loc) return 1;

    return 0;
}
static int checkErrcodeIsValid( int errcode )
{
    int ring_id, generic_idx, ring_idx;

    /* If the errcode is a class, then it is valid */
    if (errcode <= MPIR_MAX_ERROR_CLASS_INDEX && errcode >= 0) return 0;

    convertErrcodeToIndexes( errcode, &ring_idx, &ring_id, &generic_idx );
    MPL_DBG_MSG_FMT(MPIR_DBG_ERRHAND, VERBOSE, (MPL_DBG_FDEST, "code=%#010x ring_idx=%d ring_id=%#010x generic_idx=%d",
                                        errcode, ring_idx, ring_id, generic_idx));

    if (ring_idx < 0 || ring_idx >= MAX_ERROR_RING ||
	ring_idx > max_error_ring_loc) return 1;
    if (ErrorRing[ring_idx].id != ring_id) return 2;
    /* It looks like the code uses a generic idx of -1 to indicate no
       generic message */
    if (generic_idx < -1 || generic_idx > generic_msgs_len) return 3;
    return 0;
}

/* Check to see if the error code is a user-specified error code
   (e.g., from the attribute delete function) and if so, set the error code
   to the value provide by the user */
static int checkForUserErrcode( int errcode )
{
    error_ring_mutex_lock();
    {
	if (errcode != MPI_SUCCESS) {
	    int ring_idx;
	    int ring_id;
	    int generic_idx;
	    
	    if (convertErrcodeToIndexes( errcode, &ring_idx, &ring_id,
					 &generic_idx ) != 0) {
		/* --BEGIN ERROR HANDLING-- */
		MPL_error_printf( 
		  "Invalid error code (%d) (error ring index %d invalid)\n", 
		  errcode, ring_idx );
		/* --END ERROR HANDLING-- */
	    }
	    else {
		/* Can we get a more specific error message */
		if (generic_idx >= 0 && 
		    ErrorRing[ring_idx].id == ring_id && 
		    ErrorRing[ring_idx].use_user_error_code)
		    {
			errcode = ErrorRing[ring_idx].user_error_code;
		    }
	    }
	}
    }
    error_ring_mutex_unlock();
    return errcode;
}


/* --BEGIN ERROR HANDLING-- */
static const char *ErrcodeInvalidReasonStr( int reason )
{
    const char *str = 0;
    switch (reason) {
    case 1:
	str = "Ring Index out of range";
	break;
    case 2:
	str = "Ring ids do not match";
	break;
    case 3:
	str = "Generic message index out of range";
	break;
    default:
	str = "Unknown reason for invalid errcode";
	break;
    }
    return str;
}
/* --END ERROR HANDLING-- */

static void CombineSpecificCodes( int error1_code, int error2_code,
				  int error2_class )
{
    int error_code;
    
    error_code = error1_code;
    
    error_ring_mutex_lock();
    {
	for (;;)
	    {
		int error_class;
		int ring_idx;
		int ring_id;
		int generic_idx;
		
		if (convertErrcodeToIndexes(error_code, &ring_idx, &ring_id,
					    &generic_idx) != 0 || generic_idx < 0 ||
		    ErrorRing[ring_idx].id != ring_id)
		    {
			break;
		    }
		
		error_code = ErrorRing[ring_idx].prev_error;
		
		if (error_code == MPI_SUCCESS)
		    {
			ErrorRing[ring_idx].prev_error = error2_code;
			break;
		    }
		
		error_class = MPIR_ERR_GET_CLASS(error_code);
		
		if (error_class == MPI_ERR_OTHER)
		    {
			ErrorRing[ring_idx].prev_error &= ~(ERROR_CLASS_MASK);
			ErrorRing[ring_idx].prev_error |= error2_class;
		    }
	    }
    }
    error_ring_mutex_unlock();
}

static int ErrGetInstanceString( int errorcode, char msg[], int num_remaining )
{
    int len;

    if (MPIR_CVAR_PRINT_ERROR_STACK) {
	MPL_strncpy(msg, ", error stack:\n", num_remaining);
	msg[num_remaining - 1] = '\0';
	len = (int)strlen(msg);
	msg += len;
	num_remaining -= len;
	/* note: this took the "fn" arg, but that appears to be unused
	 and is undocumented.  */
	MPIR_Err_print_stack_string(errorcode, msg, num_remaining);
	msg[num_remaining - 1] = '\0';
    }
    else {
	error_ring_mutex_lock();
	{
	    while (errorcode != MPI_SUCCESS) {
		int ring_idx;
		int ring_id;
		int generic_idx;
		
		if (convertErrcodeToIndexes( errorcode, &ring_idx, 
					     &ring_id,
					     &generic_idx ) != 0) {
		    /* --BEGIN ERROR HANDLING-- */
		    MPL_error_printf( 
	      "Invalid error code (%d) (error ring index %d invalid)\n", 
	      errorcode, ring_idx );
		    break;
		    /* --END ERROR HANDLING-- */
		}
		
		if (generic_idx < 0) {
		    break;
		}
		
		if (ErrorRing[ring_idx].id == ring_id) {
		    /* just keep clobbering old values until the 
		       end of the stack is reached */
		    MPL_snprintf(msg, num_remaining, ", %s", 
				  ErrorRing[ring_idx].msg);
		    msg[num_remaining - 1] = '\0';
		    errorcode = ErrorRing[ring_idx].prev_error;
		}
		else {
		    break;
		}
	    }
	}
	error_ring_mutex_unlock();
    }
    /* FIXME: How do we determine that we failed to unwind the stack? */
    if (errorcode != MPI_SUCCESS) return 1;

    return 0;
}

#else
#error MPICH_ERROR_MSG_LEVEL undefined or has invalid value
#endif

/* Common routines that are used by two or more error-message levels.  
   Very simple routines are defined inline */
#ifdef NEEDS_FIND_GENERIC_MSG_INDEX
/* 
 * Given a message string abbreviation (e.g., one that starts "**"), return 
 * the corresponding index.  For the generic (non
 * parameterized messages), use idx = FindGenericMsgIndex( "**msg" );
 * Returns -1 on failure to find the matching message
 * 
 * The values are in increasing, sorted order, so once we find a
 * comparison where the current generic_err_msg is greater than the
 * message we are attempting to match, we have missed the match and
 * there is an internal error (all short messages should exist in defmsg.h)
 */
/* Question: Could be a service routine for message level >= generic */
static int FindGenericMsgIndex( const char msg[] )
{
    int i, c;
    for (i=0; i<generic_msgs_len; i++) {
	/* Check the sentinals to insure that the values are ok first */
	if (generic_err_msgs[i].sentinal1 != 0xacebad03 ||
	    generic_err_msgs[i].sentinal2 != 0xcb0bfa11) {
	    /* Something bad has happened! Don't risk trying the
	       short_name pointer; it may have been corrupted */
	    break;
	}
	c = strcmp( generic_err_msgs[i].short_name, msg );
	if (c == 0) return i;
	if (c > 0) {
            /* In case the generic messages are not sorted exactly the
               way that strcmp compares, we check for the case that 
               the short msg matches the current generic message.  If
               that is the case, we do *not* fail */
	    if (strncmp(generic_err_msgs[i].short_name, msg, strlen(msg)) != 0)
		return -1;
	}
    }
    /* --BEGIN ERROR HANDLING-- */
    return -1;
    /* --END ERROR HANDLING-- */
}
#endif
