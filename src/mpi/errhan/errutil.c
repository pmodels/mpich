/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
   of MPIR_Err_create_code in mpierror.h (without this definition,
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
 * Structure of this file
 *
 * This file contains several groups of routines user for error handling
 * and reporting.  
 *
 * The first group provides memory for the MPID_Errhandler objects 
 * and the routines to free and manipulate them
 *
 * MPIR_Err_return_xxx - For each of the MPI types on which an 
 * error handler can be defined, there is an MPIR_Err_return_xxx routine
 * that determines what error handler function to call and whether to 
 * abort the program.  The comm and win versions are here; ROMIO
 * provides its own routines for invoking the error handlers for Files.
 *
 * The next group of code handles the error messages.  There are four
 * options, controlled by the value of MPICH_ERROR_MSG_LEVEL. 
 *
 * MPICH_ERROR_MSG_NONE - No text messages at all
 * MPICH_ERROR_MSG_CLASS - Only messages for the MPI error classes
 * MPICH_ERROR_MSG_GENERIC - Only predefiend messages for the MPI error codes
 * MPICH_ERROR_MSG_ALL - Instance specific error messages (and error message
 *                       stack)
 *
 * In only the latter (MPICH_ERROR_MSG_ALL) case are instance-specific
 * messages maintained (including the error message "stack" that you may
 * see mentioned in various places.  In the other cases, an error code 
 * identifies a fixed message string (unless MPICH_ERROR_MSG_NONE,
 * when there are no strings) from the "generic" strings defined in defmsg.h
 *
 * A major subgroup in this section is the code to handle the instance-specific
 * messages (MPICH_ERROR_MSG_ALL only).  
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
   routines.  checkValidErrcode depends on the MPICH_ERROR_MSG_LEVEL */

static int checkValidErrcode( int error_class, const char fcname[], 
			      int *errcode );
static void handleFatalError( MPID_Comm *comm_ptr, 
			      const char fcname[], int errcode );

#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
static int ErrGetInstanceString( int errorcode, char *msg, int num_remaining );
static void MPIR_Err_stack_init( void );
static void CombineSpecificCodes( int, int, int );
static int checkForUserErrcode( int );
#else
/* We only need special handling for user error codes when we support the
   error message stack */
#define checkForUserErrcode(_a) _a
#endif /* ERROR_MSG_LEVEL >= ERROR_MSG_ALL */


/* ------------------------------------------------------------------------- */
/* Provide the MPID_Errhandler space and the routines to free and set them
   from C++  */
/* ------------------------------------------------------------------------- */
/*
 * Error handlers.  These are handled just like the other opaque objects
 * in MPICH
 */

#ifndef MPID_ERRHANDLER_PREALLOC 
#define MPID_ERRHANDLER_PREALLOC 8
#endif

/* Preallocated errorhandler objects */
MPID_Errhandler MPID_Errhandler_builtin[3] = { {0} };
MPID_Errhandler MPID_Errhandler_direct[MPID_ERRHANDLER_PREALLOC] = 
    { {0} };
MPIU_Object_alloc_t MPID_Errhandler_mem = { 0, 0, 0, 0, MPID_ERRHANDLER, 
					    sizeof(MPID_Errhandler), 
					    MPID_Errhandler_direct,
					    MPID_ERRHANDLER_PREALLOC, };

void MPID_Errhandler_free(MPID_Errhandler *errhan_ptr)
{
    MPIU_Handle_obj_free(&MPID_Errhandler_mem, errhan_ptr);
}

#ifdef HAVE_CXX_BINDING
/* This routine is used to install a callback used by the C++ binding
 to invoke the (C++) error handler.  The callback routine is a C routine,
 defined in the C++ binding. */
void MPIR_Errhandler_set_cxx( MPI_Errhandler errhand, void (*errcall)(void) )
{
    MPID_Errhandler *errhand_ptr;
    
    MPID_Errhandler_get_ptr( errhand, errhand_ptr );
    errhand_ptr->language		= MPID_LANG_CXX;
    MPIR_Process.cxx_call_errfn	= (void (*)( int, int *, int *, 
					    void (*)(void) ))errcall;
}
#endif /* HAVE_CXX_BINDING */

#if defined(HAVE_FORTRAN_BINDING) && !defined(HAVE_FINT_IS_INT)
void MPIR_Errhandler_set_fc( MPI_Errhandler errhand )
{
    MPID_Errhandler *errhand_ptr;
    
    MPID_Errhandler_get_ptr( errhand, errhand_ptr );
    errhand_ptr->language = MPID_LANG_FORTRAN;
}

#endif
/* ------------------------------------------------------------------------- */
/* These routines are called on error exit from most top-level MPI routines
   to invoke the appropriate error handler.  Also included is the routine
   to call if MPI has not been initialized (MPIR_Err_preinit) and to 
   determine if an error code represents a fatal error (MPIR_Err_is_fatal). */
/* ------------------------------------------------------------------------- */
/* Special error handler to call if we are not yet initialized, or if we
   have finalized */
void MPIR_Err_preOrPostInit( void )
{
    if (MPIR_Process.initialized == MPICH_PRE_INIT) {
	MPIU_Error_printf("Attempting to use an MPI routine before initializing MPICH\n");
    }
    else if (MPIR_Process.initialized == MPICH_POST_FINALIZED) {
	MPIU_Error_printf("Attempting to use an MPI routine after finalizing MPICH\n");
    }
    else {
	MPIU_Error_printf("Internal Error: Unknown state of MPI (neither initialized nor finalized)\n" );
    }
    exit(1);
}

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
int MPIR_Err_return_comm( MPID_Comm  *comm_ptr, const char fcname[], 
			  int errcode )
{
    const int error_class = ERROR_GET_CLASS(errcode);
    MPID_Errhandler *errhandler = NULL;
    int rc;

    rc = checkValidErrcode( error_class, fcname, &errcode );

    if (MPIR_Process.initialized == MPICH_PRE_INIT ||
        MPIR_Process.initialized == MPICH_POST_FINALIZED)
    {
        /* for whatever reason, we aren't initialized (perhaps error 
	   during MPI_Init) */
        handleFatalError(MPIR_Process.comm_world, fcname, errcode);
        return MPI_ERR_INTERN;
    }

    MPIU_DBG_MSG_FMT(ERRHAND, TERSE, (MPIU_DBG_FDEST, "MPIR_Err_return_comm(comm_ptr=%p, fcname=%s, errcode=%d)", comm_ptr, fcname, errcode));

    if (comm_ptr) {
        MPIU_THREAD_CS_ENTER(MPI_OBJ, comm_ptr);
        errhandler = comm_ptr->errhandler;
        MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);
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

    if (MPIR_Err_is_fatal(errcode) || comm_ptr == NULL) {
	/* Calls MPID_Abort */
	handleFatalError( comm_ptr, fcname, errcode );
        /* never get here */
    }

    MPIU_Assert(comm_ptr != NULL);

    /* comm_ptr may have changed to comm_world.  Keep this locked as long as we
     * are using the errhandler to prevent it from disappearing out from under
     * us. */
    MPIU_THREAD_CS_ENTER(MPI_OBJ, comm_ptr);
    errhandler = comm_ptr->errhandler;

    if (errhandler == NULL || errhandler->handle == MPI_ERRORS_ARE_FATAL) {
        MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);
	/* Calls MPID_Abort */
	handleFatalError( comm_ptr, fcname, errcode );
        /* never get here */
    }

    /* Check for the special case of a user-provided error code */
    errcode = checkForUserErrcode( errcode );

    if (errhandler->handle != MPI_ERRORS_RETURN &&
        errhandler->handle != MPIR_ERRORS_THROW_EXCEPTIONS)
    {
	/* We pass a final 0 (for a null pointer) to these routines
	   because MPICH-1 expected that */
	switch (comm_ptr->errhandler->language)
	{
	case MPID_LANG_C:
	    (*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&comm_ptr->handle, &errcode, 0 );
	    break;
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX:
	    (*MPIR_Process.cxx_call_errfn)( 0, &comm_ptr->handle, &errcode, 
		    (void (*)(void))*comm_ptr->errhandler->errfn.C_Comm_Handler_function );
	    /* The C++ code throws an exception if the error handler 
	     returns something other than MPI_SUCCESS. There is no "return"
	     of an error code. */
	    errcode = MPI_SUCCESS;
	    break;
#endif /* CXX_BINDING */
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
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

    MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);
    return errcode;
}

/* 
 * MPI routines that detect errors on window objects use this to report errors
 */
int MPIR_Err_return_win( MPID_Win  *win_ptr, const char fcname[], int errcode )
{
    const int error_class = ERROR_GET_CLASS(errcode);
    int rc ;

    if (win_ptr == NULL || win_ptr->errhandler == NULL)
	return MPIR_Err_return_comm(NULL, fcname, errcode);

    /* We don't test for MPI initialized because to call this routine,
       we will have had to call an MPI routine that would make that test */

    rc = checkValidErrcode( error_class, fcname, &errcode );

    MPIU_DBG_MSG_FMT(ERRHAND, TERSE, (MPIU_DBG_FDEST, "MPIR_Err_return_win(win_ptr=%p, fcname=%s, errcode=%d)", win_ptr, fcname, errcode));

    if (MPIR_Err_is_fatal(errcode) ||
	win_ptr == NULL || win_ptr->errhandler == NULL || 
	win_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
	/* Calls MPID_Abort */
	handleFatalError( NULL, fcname, errcode );
    }

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
	    case MPID_LANG_C:
		(*win_ptr->errhandler->errfn.C_Win_Handler_function)( 
		    &win_ptr->handle, &errcode, 0 );
		break;
#ifdef HAVE_CXX_BINDING
	    case MPID_LANG_CXX:
	    (*MPIR_Process.cxx_call_errfn)( 2, &win_ptr->handle, &errcode, 
		    (void (*)(void))*win_ptr->errhandler->errfn.C_Win_Handler_function );
	    /* The C++ code throws an exception if the error handler 
	     returns something other than MPI_SUCCESS. There is no "return"
	     of an error code. */
	    errcode = MPI_SUCCESS;
	    break;
#endif /* CXX_BINDING */
#ifdef HAVE_FORTRAN_BINDING
	    case MPID_LANG_FORTRAN90:
	    case MPID_LANG_FORTRAN:
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

void MPIR_Err_init( void )
{
    /* these are "stub" objects, so the other fields (which are statically
     * initialized to zero) don't really matter */
    MPID_Errhandler_builtin[0].handle = MPI_ERRORS_ARE_FATAL;
    MPID_Errhandler_builtin[1].handle = MPI_ERRORS_RETURN;
    MPID_Errhandler_builtin[2].handle = MPIR_ERRORS_THROW_EXCEPTIONS;

#   if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
    MPIR_Err_stack_init();
#   endif
    did_err_init = TRUE;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
static int checkErrcodeIsValid( int errcode );
static const char *ErrcodeInvalidReasonStr( int reason );
static const char *get_class_msg( int error_class );


/* ------------------------------------------------------------------------- */
/* The following block of code manages the instance-specific error messages  */
/* ------------------------------------------------------------------------- */

 
/* Check for a valid error code.  If the code is not valid, attempt to
   print out something sensible; reset the error code to have class 
   ERR_UNKNOWN */
/* FIXME: Now that error codes are chained, this does not produce a valid 
   error code since there is no valid ring index corresponding to this code */
static int checkValidErrcode( int error_class, const char fcname[], 
			      int *errcode_p )
{
    int errcode = *errcode_p;
    int rc = 0;

    if (error_class > MPICH_ERR_LAST_CLASS)
    {
	if (errcode & ~ERROR_CLASS_MASK)
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n"
			      "%s.  Please file a bug report.\n", error_class, fcname);
	    /* Note that we don't try to print the error stack; if the 
	       error code is invalid, it can't be used to find
	       the error stack.  We could consider dumping the 
	       contents of the error ring instead (without trying
	       to interpret them) */
	}
	else
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n"
			      "%s.  Please file a bug report.  No error stack is available.\n", error_class, fcname);
	}
	/* FIXME: We probably want to set this to MPI_ERR_UNKNOWN
	   and discard the rest of the bits */
	errcode = (errcode & ~ERROR_CLASS_MASK) | MPI_ERR_UNKNOWN;
	rc = 1;
    }
    *errcode_p = errcode;
    return rc;
}

/* Check that an encoded error code is valid. Return 0 if valid, positive, 
   non-zero if invalid.  Value indicates reason; see 
   ErrcodeInvalidReasonStr() */

#if MPICH_ERROR_MSG_LEVEL <= MPICH_ERROR_MSG_GENERIC
/* This is the shortened version when there are no error messages */
static int checkErrcodeIsValid( int errcode )
{
    /* MPICH_ERR_LAST_CLASS is the last of the *predefined* error classes. */
    /* FIXME: Should this check against dynamically-created error classes? */
    if (errcode < 0 || errcode >= MPICH_ERR_LAST_CLASS) {
	return 3;
    }
    return 0;
}
static const char *ErrcodeInvalidReasonStr( int reason )
{
    const char *str = 0;

    /* FIXME: These strings need to be internationalized */
    switch (reason) {
    case 3:
	str = "Message class is out of range";
	break;
    default:
	str = "Unknown reason for invalid errcode";
	break;
    }
    return str;
}
#endif /* MPICH_ERROR_MSG_LEVEL <= MPICH_ERROR_MSG_GENERIC */

/* This routine is called when there is a fatal error */
static void handleFatalError( MPID_Comm *comm_ptr, 
			      const char fcname[], int errcode )
{
    /* Define length of the the maximum error message line (or string with 
       newlines?).  This definition is used only within this routine.  */
    /* FIXME: This should really be the same as MPI_MAX_ERROR_STRING, or in the
       worst case, defined in terms of that */
#define MAX_ERRMSG_STRING 4096
    char error_msg[ MAX_ERRMSG_STRING ];
    int len;

    /* FIXME: Not internationalized */
    MPIU_Snprintf(error_msg, MAX_ERRMSG_STRING, "Fatal error in %s: ", fcname);
    len = (int)strlen(error_msg);
    MPIR_Err_get_string(errcode, &error_msg[len], MAX_ERRMSG_STRING-len, NULL);
    /* The third argument is a return code, a value of 1 usually indicates
       an error */
    MPID_Abort(comm_ptr, MPI_SUCCESS, 1, error_msg);
}

#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_GENERIC
/* 
 * Given a message string abbreviation (e.g., one that starts "**"), return 
 * the corresponding index.  For the generic (non
 * parameterized messages), use idx = FindGenericMsgIndex( "**msg" );
 */
static int FindGenericMsgIndex( const char *msg )
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
	if (c > 0)
	{
	    /* don't return here if the string partially matches */
	    if (strncmp(generic_err_msgs[i].short_name, msg, strlen(msg)) != 0)
		return -1;
	}
    }
    return -1;
}

/* 
   Here is an alternate search routine based on bisection.
   int i_low, i_mid, i_high, c;
   i_low = 0; i_high = generic_msg_len - 1;
   while (i_high - i_low >= 0) {
       i_mid = (i_high + i_low) / 2;
       c = strcmp( generic_err_msgs[i].short_name, msg );
       if (c == 0) return i_mid;
       if (c < 0) { i_low = i_mid + 1; }
       else       { i_high = i_mid - 1; }
   }
   return -1;
*/
#endif /* MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_GENERIC */

#if MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG_ALL
#endif /* MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG_ALL */


/* ------------------------------------------------------------------------ */
/* The following routines create an MPI error code, handling optional,      */
/* instance-specific error message information.  There are two key routines:*/
/*    MPIR_Err_create_code - Create the error code; this is the routine used*/
/*                           by most routines                               */
/*    MPIR_Err_create_code_valist - Create the error code; accept a valist  */
/*                           instead of a variable argument list (this is   */
/*                           used to allow this routine to be used from     */
/*                           within another varargs routine)                */
/* ------------------------------------------------------------------------ */

/* Err_create_code is just a shell that accesses the va_list and then
   calls the real routine.  */
int MPIR_Err_create_code( int lastcode, int fatal, const char fcname[], 
			  int line, int error_class, const char generic_msg[],
			  const char specific_msg[], ... )
{
    int rc;
    va_list Argp;
    va_start(Argp, specific_msg);
    MPIU_DBG_MSG_FMT(ERRHAND, TYPICAL, (MPIU_DBG_FDEST, "%sError created: %s(%d) %s", fatal ? "Fatal " : "",
                                        fcname, line, generic_msg));
    rc = MPIR_Err_create_code_valist( lastcode, fatal, fcname, line,
				      error_class, generic_msg, specific_msg,
				      Argp );
    va_end(Argp);
    return rc;
}

#if MPICH_ERROR_MSG_LEVEL < MPICH_ERROR_MSG_ALL
/* In this case, the routine ignores all but (possibly) the generic message.
   It also returns lastcode *unless* lastcode is MPI_SUCCESS.  Thus, the 
   error returned is the error specified at the point of detection.
 */
int MPIR_Err_create_code_valist( int lastcode, int fatal, const char fcname[], 
				 int line, int error_class, 
				 const char generic_msg[],
				 const char specific_msg[], va_list Argp )
{
#if MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG_GENERIC
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
#else
    int errcode = lastcode;
    if (lastcode == MPI_SUCCESS) errcode = error_class;
    return errcode;
#endif /* MSG_LEVEL == GENERIC */
}
/* FIXME: Factor the error routines so that there is a single version of each */
int MPIR_Err_combine_codes(int error1, int error2)
{
    int error1_code = error1;
    int error2_code = error2;
    int error2_class;
    
    if (error2_code == MPI_SUCCESS) return error1_code;
    /* Return the error2 code if either it is a user-defined error or
       error 1 is no-error */
    if (error2_code & ERROR_DYN_MASK) return error2_code;
    if (error1_code == MPI_SUCCESS) return error2_code;
	    
    error2_class = MPIR_ERR_GET_CLASS(error2_code);
    if (MPIR_ERR_GET_CLASS(error2_class) < MPI_SUCCESS ||
	MPIR_ERR_GET_CLASS(error2_class) > MPICH_ERR_LAST_CLASS)
    {
	error2_class = MPI_ERR_OTHER;
    }

    /* If the class of error1 is OTHER, replace it with the class of
       error2 */
    if (MPIR_ERR_GET_CLASS(error1_code) == MPI_ERR_OTHER)
    {
	error1_code = (error1_code & ~(ERROR_CLASS_MASK)) | error2_class;
    }

    return error1_code;
}

#endif /* msg_level < msg_all */

/*
 * Accessor routines for the predefined mqessages.  These can be
 * used by the other routines (such as MPI_Error_string) to
 * access the messages in this file, or the messages that may be
 * available through any message catalog facility 
 */
static const char *get_class_msg( int error_class )
{
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
    if (error_class >= 0 && error_class < MPIR_MAX_ERROR_CLASS_INDEX) {
#if MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG_CLASS
	return classToMsg[error_class];
#else
	return generic_err_msgs[class_to_index[error_class]].long_name;
#endif
    }
    else {
	return "Unknown error class";
    }
#else 
    /* FIXME: Not internationalized */
    return "Error message texts are not available";
#endif /* MSG_LEVEL > MSG_NONE */
}

/* FIXME: This routine isn't quite right yet */
/*
 * Notes:
 * One complication is that in the instance-specific case, a ??
 */
void MPIR_Err_get_string( int errorcode, char * msg, int length, 
			  MPIR_Err_get_class_string_func_t fn )
{
    int error_class;
    int len, num_remaining = length;
    
    if (num_remaining == 0)
	num_remaining = MPI_MAX_ERROR_STRING;

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
	   MPI_Err_add_class) */
	/* If a dynamic error code was created, the function to convert
	   them into strings has been set.  Check to see that it was; this 
	   is a safeguard against a bogus error code */
	if (!MPIR_Process.errcode_to_string) {
	    /* FIXME: not internationalized */
	    /* --BEGIN ERROR HANDLING-- */
	    if (MPIU_Strncpy(msg, "Undefined dynamic error code", 
			     num_remaining))
	    {
		msg[num_remaining - 1] = '\0';
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    if (MPIU_Strncpy(msg, MPIR_Process.errcode_to_string( errorcode ), 
			     num_remaining))
	    {
		msg[num_remaining - 1] = '\0';
	    }
	}
    }
    else if ( (errorcode & ERROR_CLASS_MASK) == errorcode) {
	error_class = MPIR_ERR_GET_CLASS(errorcode);

	/* FIXME: Why was the last test commented out? */
	if (fn != NULL && error_class > MPICH_ERR_LAST_CLASS /*&& error_class < MPICH_ERR_MAX_EXT_CLASS*/)
	{
	    fn(errorcode, msg, length);
	}
	else
	{
	    if (MPIU_Strncpy(msg, get_class_msg( errorcode ), num_remaining))
	    {
		msg[num_remaining - 1] = '\0';
	    }
	}
    }
    else
    {
	/* print the class message first */
	error_class = MPIR_ERR_GET_CLASS(errorcode);

	/* FIXME: Why was the last test commented out? */
	if (fn != NULL && error_class > MPICH_ERR_LAST_CLASS /*&& error_class < MPICH_ERR_MAX_EXT_CLASS*/)
	{
	    fn(errorcode, msg, num_remaining);
	}
	else
	{
	    MPIU_Strncpy(msg, get_class_msg(ERROR_GET_CLASS(errorcode)), num_remaining);
	}
	msg[num_remaining - 1] = '\0';
	len = (int)strlen(msg);
	msg += len;
	num_remaining -= len;

	/* then print the stack or the last specific error message */

#       if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
	if (ErrGetInstanceString( errorcode, msg, num_remaining )) 
	    goto fn_exit;
#elif MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_CLASS
	{
	    int generic_idx;
	    
	    generic_idx = ((errorcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;
	    
	    if (generic_idx >= 0) {
		MPIU_Snprintf(msg, num_remaining, ", %s", generic_err_msgs[generic_idx].long_name);
		msg[num_remaining - 1] = '\0';
		goto fn_exit;
	    }
	}
#           endif /* MSG_LEVEL >= MSG_ALL */
    }
    
fn_exit:
    return;
}

/* 
 * If the error message level is all, MPICH2 supports instance-specific
 * error messages.  Details above
 */
#if MPICH_ERROR_MSG_LEVEL == MPICH_ERROR_MSG_ALL

static int convertErrcodeToIndexes( int errcode, int *ring_idx, int *ring_id,
				    int *generic_idx );



/* FIXME: Where is the documentation for this function?  What is it for?  */
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
   MPIU_THREAD_CHECK macros are not appropriate */
static MPID_Thread_mutex_t error_ring_mutex;
#define error_ring_mutex_create(_mpi_errno_p_)			\
    MPID_Thread_mutex_create(&error_ring_mutex, _mpi_errno_p_)
#define error_ring_mutex_destroy(_mpi_errno_p)				\
    MPID_Thread_mutex_destroy(&error_ring_mutex, _mpi_errno_p_)
#define error_ring_mutex_lock()			\
    MPID_Thread_mutex_lock(&error_ring_mutex)
#define error_ring_mutex_unlock()		\
    MPID_Thread_mutex_unlock(&error_ring_mutex)
#elif defined(MPICH_IS_THREADED)
static MPID_Thread_mutex_t error_ring_mutex;
#define error_ring_mutex_create(_mpi_errno_p) MPID_Thread_mutex_create(&error_ring_mutex,_mpi_errno_p)
#define error_ring_mutex_destroy(_mpi_errno_p) MPID_Thread_mutex_destroy(&error_ring_mutex,_mpi_errno_p)
#define error_ring_mutex_lock()                          \
    do {                                                 \
        if (did_err_init) {                              \
            MPIU_THREAD_CHECK_BEGIN                      \
            MPID_Thread_mutex_lock(&error_ring_mutex);   \
            MPIU_THREAD_CHECK_END                        \
        }                                                \
    } while (0)
#define error_ring_mutex_unlock()                        \
    do {                                                 \
        if (did_err_init) {                              \
            MPIU_THREAD_CHECK_BEGIN                      \
            MPID_Thread_mutex_unlock(&error_ring_mutex); \
            MPIU_THREAD_CHECK_END                        \
        }                                                \
    } while (0)
#else
#define error_ring_mutex_create(_a)
#define error_ring_mutex_destroy(_a)
#define error_ring_mutex_lock()
#define error_ring_mutex_unlock()
#endif /* REQUIRES_THREAD_SAFETY */

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
    if (ring_idx < 0 || ring_idx >= MAX_ERROR_RING ||
	ring_idx > max_error_ring_loc) return 1;
    if (ErrorRing[ring_idx].id != ring_id) return 2;
    /* It looks like the code uses a generic idx of -1 to indicate no
       generic message */
    if (generic_idx < -1 || generic_idx > generic_msgs_len) return 3;
    return 0;
}
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
		MPIU_Error_printf( 
		  "Invalid error code (%d) (error ring index %d invalid)\n", 
		  errcode, ring_idx );
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

/* 
 * Given a message string abbreviation (e.g., one that starts "**"), return 
 * the corresponding index.  For the specific
 * (parameterized messages), use idx = FindSpecificMsgIndex( "**msg" );
 */
static int FindSpecificMsgIndex( const char *msg )
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
	MPIU_Strncpy(str, "assert=0", ASSERT_STR_MAXLEN);
	return str;
    }
    cur = str;
    if (d & MPI_MODE_NOSTORE)
    {
	MPIU_Strncpy(cur, "MPI_MODE_NOSTORE", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOSTORE;
    }
    if (d & MPI_MODE_NOCHECK)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPIU_Strncpy(cur, " | MPI_MODE_NOCHECK", len);
	else
	    MPIU_Strncpy(cur, "MPI_MODE_NOCHECK", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOCHECK;
    }
    if (d & MPI_MODE_NOPUT)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPIU_Strncpy(cur, " | MPI_MODE_NOPUT", len);
	else
	    MPIU_Strncpy(cur, "MPI_MODE_NOPUT", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOPUT;
    }
    if (d & MPI_MODE_NOPRECEDE)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPIU_Strncpy(cur, " | MPI_MODE_NOPRECEDE", len);
	else
	    MPIU_Strncpy(cur, "MPI_MODE_NOPRECEDE", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOPRECEDE;
    }
    if (d & MPI_MODE_NOSUCCEED)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPIU_Strncpy(cur, " | MPI_MODE_NOSUCCEED", len);
	else
	    MPIU_Strncpy(cur, "MPI_MODE_NOSUCCEED", len);
	n = strlen(cur);
	cur += n;
	len -= n;
	d ^= MPI_MODE_NOSUCCEED;
    }
    if (d)
    {
	if (len < ASSERT_STR_MAXLEN)
	    MPIU_Snprintf(cur, len, " | 0x%x", d);
	else
	    MPIU_Snprintf(cur, len, "assert=0x%x", d);
    }
    return str;
}

static const char * GetDTypeString(MPI_Datatype d)
{
    static char default_str[64];
    int num_integers, num_addresses, num_datatypes, combiner = 0;
    char *str;

    if (HANDLE_GET_MPI_KIND(d) != MPID_DATATYPE ||      \
	(HANDLE_GET_KIND(d) == HANDLE_KIND_INVALID &&   \
	d != MPI_DATATYPE_NULL))
        return "INVALID DATATYPE";
    

    if (d == MPI_DATATYPE_NULL)
	return "MPI_DATATYPE_NULL";

    if (d == 0)
    {
	MPIU_Strncpy(default_str, "dtype=0x0", sizeof(default_str));
	return default_str;
    }

    MPID_Type_get_envelope(d, &num_integers, &num_addresses, &num_datatypes, 
			   &combiner);
    if (combiner == MPI_COMBINER_NAMED)
    {
	str = MPIDU_Datatype_builtin_to_string(d);
	if (str == NULL)
	{
	    MPIU_Snprintf(default_str, sizeof(default_str), "dtype=0x%08x", d);
	    return default_str;
	}
	return str;
    }
    
    /* default is not thread safe */
    str = MPIDU_Datatype_combiner_to_string(combiner);
    if (str == NULL)
    {
	MPIU_Snprintf(default_str, sizeof(default_str), "dtype=USER<0x%08x>", d);
	return default_str;
    }
    MPIU_Snprintf(default_str, sizeof(default_str), "dtype=USER<%s>", str);
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
    }
    /* FIXME: default is not thread safe */
    MPIU_Snprintf(default_str, sizeof(default_str), "op=0x%x", o);
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
    void *p;

    fmt = MPIU_Strdup(fmt_orig);
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
	    MPIU_Memcpy(str, begin, len);
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
	        MPIU_Strncpy(str, s, maxlen);
            else {
		MPIU_Strncpy(str, "<NULL>", maxlen );
	    }
	    break;
	case (int)'d':
	    d = va_arg(list, int);
	    MPIU_Snprintf(str, maxlen, "%d", d);
	    break;
	case (int)'L':
	    ll = va_arg(list, long long);
	    MPIU_Snprintf(str, maxlen, "%lld", ll);
	    break;
        case (int)'x':
            d = va_arg(list, int);
            MPIU_Snprintf(str, maxlen, "%x", d);
            break;
        case (int)'X':
            ll = va_arg(list, long long);
            MPIU_Snprintf(str, maxlen, "%llx", ll);
            break;
	case (int)'i':
	    i = va_arg(list, int);
	    switch (i)
	    {
	    case MPI_ANY_SOURCE:
		MPIU_Strncpy(str, "MPI_ANY_SOURCE", maxlen);
		break;
	    case MPI_PROC_NULL:
		MPIU_Strncpy(str, "MPI_PROC_NULL", maxlen);
		break;
	    case MPI_ROOT:
		MPIU_Strncpy(str, "MPI_ROOT", maxlen);
		break;
	    default:
		MPIU_Snprintf(str, maxlen, "%d", i);
		break;
	    }
	    break;
	case (int)'t':
	    t = va_arg(list, int);
	    switch (t)
	    {
	    case MPI_ANY_TAG:
		MPIU_Strncpy(str, "MPI_ANY_TAG", maxlen);
		break;
		/* FIXME: Is MPI_UNDEFINED valid as a tag? */
	    case MPI_UNDEFINED:
		MPIU_Strncpy(str, "MPI_UNDEFINED", maxlen);
		break;
	    default:
		MPIU_Snprintf(str, maxlen, "%d", t);
		break;
	    }
	    break;
	case (int)'p':
	    p = va_arg(list, void *);
	    /* FIXME: A check for MPI_IN_PLACE should only be used 
	       where that is valid */
	    if (p == MPI_IN_PLACE)
	    {
		MPIU_Strncpy(str, "MPI_IN_PLACE", maxlen);
	    }
	    else
	    {
		/* FIXME: We may want to use 0x%p for systems that 
		   (including Windows) that don't prefix %p with 0x. 
		   This must be done with a capability, not a test on
		   particular OS or header files */
		MPIU_Snprintf(str, maxlen, "%p", p);
	    }
	    break;
	case (int)'C':
	    C = va_arg(list, MPI_Comm);
	    switch (C)
	    {
	    case MPI_COMM_WORLD:
		MPIU_Strncpy(str, "MPI_COMM_WORLD", maxlen);
		break;
	    case MPI_COMM_SELF:
		MPIU_Strncpy(str, "MPI_COMM_SELF", maxlen);
		break;
	    case MPI_COMM_NULL:
		MPIU_Strncpy(str, "MPI_COMM_NULL", maxlen);
		break;
	    default:
		MPIU_Snprintf(str, maxlen, "comm=0x%x", C);
		break;
	    }
	    break;
	case (int)'I':
	    info = va_arg(list, MPI_Info);
	    if (info == MPI_INFO_NULL)
	    {
		MPIU_Strncpy(str, "MPI_INFO_NULL", maxlen);
	    }
	    else
	    {
		MPIU_Snprintf(str, maxlen, "info=0x%x", info);
	    }
	    break;
	case (int)'D':
	    D = va_arg(list, MPI_Datatype);
	    MPIU_Snprintf(str, maxlen, "%s", GetDTypeString(D));
	    break;
	    /* Include support for %F only if MPI-IO is enabled */
#ifdef MPI_MODE_RDWR
	case (int)'F':
	    {
	    MPI_File F;
	    F = va_arg(list, MPI_File);
	    if (F == MPI_FILE_NULL)
	    {
		MPIU_Strncpy(str, "MPI_FILE_NULL", maxlen);
	    }
	    else
	    {
		MPIU_Snprintf(str, maxlen, "file=0x%lx", (unsigned long)F);
	    }
	    }
	    break;
#endif /* MODE_RDWR */
	case (int)'W':
	    W = va_arg(list, MPI_Win);
	    if (W == MPI_WIN_NULL)
	    {
		MPIU_Strncpy(str, "MPI_WIN_NULL", maxlen);
	    }
	    else
	    {
		MPIU_Snprintf(str, maxlen, "win=0x%x", W);
	    }
	    break;
	case (int)'A':
	    d = va_arg(list, int);
	    MPIU_Snprintf(str, maxlen, "%s", GetAssertString(d));
	    break;
	case (int)'G':
	    G = va_arg(list, MPI_Group);
	    if (G == MPI_GROUP_NULL)
	    {
		MPIU_Strncpy(str, "MPI_GROUP_NULL", maxlen);
	    }
	    else
	    {
		MPIU_Snprintf(str, maxlen, "group=0x%x", G);
	    }
	    break;
	case (int)'O':
	    O = va_arg(list, MPI_Op);
	    MPIU_Snprintf(str, maxlen, "%s", GetMPIOpString(O));
	    break;
	case (int)'R':
	    R = va_arg(list, MPI_Request);
	    if (R == MPI_REQUEST_NULL)
	    {
		MPIU_Strncpy(str, "MPI_REQUEST_NULL", maxlen);
	    }
	    else
	    {
		MPIU_Snprintf(str, maxlen, "req=0x%x", R);
	    }
	    break;
	case (int)'E':
	    E = va_arg(list, MPI_Errhandler);
	    if (E == MPI_ERRHANDLER_NULL)
	    {
		MPIU_Strncpy(str, "MPI_ERRHANDLER_NULL", maxlen);
	    }
	    else
	    {
		MPIU_Snprintf(str, maxlen, "errh=0x%x", E);
	    }
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
	MPIU_Strncpy(str, begin, maxlen);
    }
    /* Free the dup'ed format string */
    MPIU_Free( fmt );

    return mpi_errno;
}

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


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/*
 * Instance-specific error messages are stored in a ring.  The elements of this
 * ring are MPIR_Err_msg_t structures, which contain the following fields:
 *    id - this is used to check that the entry is valid; it is computed from
 *         the error code and location in the ring.  The routine
 *           ErrcodeToId( errcode, &id ) is used to extract the id from an 
 *         error code and
 *           ErrcodeCreateID( class, generic, msg, &id, &seq ) is used to 
 *         create the id from an error class, generic index, and message 
 *         string.  The "seq" field is inserted into the error code as a 
 *         check.
 *
 *    prev_error - The full MPI error code of the previous error attached 
 *         to this list of errors, or MPI_SUCCESSS (which has value 0).
 *         This is the last error code, not the index in the ring of the last 
 *         error code.  That's the right choice, because we want to ensure 
 *         that the value is valid if the ring overflows.  In addition,
 *         we allow this to be an error CLASS (one of the predefined MPI
 *         error classes).  This is particularly important for 
 *         MPI_ERR_IN_STATUS, which may be returned as a valid error code.
 *         (classes are valid error codes).
 *         
 *    use_user_error_code and user_error_code - Used to handle a few cases 
 *         in MPI where a user-provided routine returns an error code; 
 *         this allows us to provide information about the chain of 
 *         routines that were involved, while returning the users prefered
 *         error value to the users environment.  See the note below
 *         on user error codes.
 *
 *    location - A string that indicates what function and line number
 *         where the error code was set.
 *
 *    msg - A message about the error.  This may be instance-specific (e.g.,
 *         it may have been created at the time the error was detected with
 *         information about the parameters that caused the error).
 *
 * Note that both location and msg are defined as length MAX_xxx+1.  This 
 * isn't really necessary (at least for msg), since the MPI standard 
 * requires that MAX_MPI_ERROR_STRING include the space for the trailing null,
 * but using the extra byte makes the code a little simpler.
 *
 * The "id" value is used to keep a sort of "checkvalue" to ensure that the
 * error code that points at this message is in fact for this particular 
 * message.  This is used to handle the unlikely but possible situation where 
 * so many error messages are generated that the ring is overlapped.
 *
 * The message arrays are preallocated to ensure that there is space for these
 * messages when an error occurs.  One variation would be to allow these
 * to be dynamically allocated, but it is probably better to either preallocate
 * these or turn off all error message generation (which will eliminate these
 * arrays).
 *
 * One possible alternative is to use the message ring *only* for instance
 * messages and use the predefined messages in-place for the generic
 * messages.  The approach used here provides uniform handling of all 
 * error messages.
 *
 * Note on user error codes
 *
 * The "user error codes" is used to handle an ambiguity in the MPI-1 
 * standard about the return value from the attribute callbacks.  The 
 * standard does not specify what values, other than 'MPI_SUCCESS', are
 * valid.  Because the Intel MPI-1 test suite expected 'MPI_Comm_dup' to
 * return the same non-zero value returned by the attribute callback routine,
 * this is the behavior that many (if not all) MPI implementations provide.
 * As a result, the return from those routines is 
 * 
 */


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
	    MPIU_Error_printf( "Internal Error: invalid error code %x (%s) in %s:%d\n", 
			       lastcode, ErrcodeInvalidReasonStr( reason ), 
			       fcname, line );
	    lastcode = MPI_SUCCESS;
	}
    }

    /* FIXME: ERR_OTHER is overloaded; this may mean "OTHER" or it may
       mean "No additional error, just routine stack info" */
    if (error_class == MPI_ERR_OTHER)
    {
        if (MPIR_ERR_GET_CLASS(lastcode) > MPI_SUCCESS && 
	    MPIR_ERR_GET_CLASS(lastcode) <= MPICH_ERR_LAST_CLASS)
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
		MPIU_Snprintf( user_ring_msg, sizeof(user_ring_msg),
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
		    /* FIXME : Internal error.  Generate some debugging 
		       information; Fix for the general release */
		    fprintf( stderr, "Could not find %s in list of messages\n", generic_msg );
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
		    MPIU_Strncpy( ring_msg, user_ring_msg, MPIR_MAX_ERROR_LINE );
		}
	    }
	    else if (generic_idx >= 0)
	    {
		MPIU_Strncpy( ring_msg,generic_err_msgs[generic_idx].long_name,
			      MPIR_MAX_ERROR_LINE );
	    }
	    else
	    {
		MPIU_Strncpy( ring_msg, generic_msg, MPIR_MAX_ERROR_LINE );
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
		    MPIU_Error_printf( 
		  "Invalid error code (%d) (error ring index %d invalid)\n", 
		  lastcode, last_ring_idx );
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
		MPIU_Snprintf(ErrorRing[ring_idx].location, MAX_LOCATION_LEN, "%s(%d)", fcname, line);
		ErrorRing[ring_idx].location[MAX_LOCATION_LEN] = '\0';
	    }
	    else
	    {
		ErrorRing[ring_idx].location[0] = '\0';
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
    
    if (error2_code == MPI_SUCCESS) return error1_code;
    if (error2_code & ERROR_DYN_MASK) return error2_code;
    if (error1_code == MPI_SUCCESS) return error2_code;
	    
    error2_class = MPIR_ERR_GET_CLASS(error2_code);
    if (MPIR_ERR_GET_CLASS(error2_class) < MPI_SUCCESS ||
	MPIR_ERR_GET_CLASS(error2_class) > MPICH_ERR_LAST_CLASS)
    {
	error2_class = MPI_ERR_OTHER;
    }

    CombineSpecificCodes( error1_code, error2_code, error2_class );

    if (MPIR_ERR_GET_CLASS(error1_code) == MPI_ERR_OTHER)
    {
	error1_code = (error1_code & ~(ERROR_CLASS_MASK)) | error2_class;
    }

    return error1_code;
}


/* ------------------------------------------------------------------------- */
/* Manage the error reporting stack                                          */
/* ------------------------------------------------------------------------- */

static void MPIR_Err_stack_init( void )
{
    int mpi_errno = MPI_SUCCESS;

    error_ring_mutex_create(&mpi_errno);

    if (MPIR_PARAM_CHOP_ERROR_STACK < 0) {
        MPIR_PARAM_CHOP_ERROR_STACK = 80;
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
                    MPIR_PARAM_CHOP_ERROR_STACK = info.dwMaximumWindowSize.X;
                }
            }
        }
#endif /* WINDOWS_H */
    }
}

/**/
/* Given an error code, print the stack of messages corresponding to this
   error code. */
void MPIR_Err_print_stack(FILE * fp, int errcode)
{
    error_ring_mutex_lock();
    {
	while (errcode != MPI_SUCCESS) {
	    int ring_idx;
	    int ring_id;
	    int generic_idx;
	    
	    if (convertErrcodeToIndexes( errcode, &ring_idx, &ring_id,
					 &generic_idx ) != 0) {
		MPIU_Error_printf( 
		    "Invalid error code (%d) (error ring index %d invalid)\n", 
		    errcode, ring_idx );
		break;
	    }
	    
	    if (generic_idx < 0)
	    {
		break;
	    }
	    
	    if (ErrorRing[ring_idx].id == ring_id)
	    {
		fprintf(fp, "%s: %s\n", ErrorRing[ring_idx].location, 
			ErrorRing[ring_idx].msg);
		errcode = ErrorRing[ring_idx].prev_error;
	    }
	    else
	    {
		break;
	    }
	}
    }
    error_ring_mutex_unlock();
	
    /* FIXME: This is wrong.  The only way that you can get here without
       errcode being MPI_SUCCESS is if there is an error in the 
       processing of the error codes.  Dropping through into the next
       level of code (particularly when that code doesn't check for 
       valid error codes!) is erroneous */
    if (errcode == MPI_SUCCESS)
    {
	goto fn_exit;
    }

    {
	int generic_idx;
		    
	generic_idx = ((errcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;
	
	if (generic_idx >= 0)
	{
	    fprintf(fp, "(unknown)(): %s\n", generic_err_msgs[generic_idx].long_name);
	    goto fn_exit;
	}
    }
    
    {
	int error_class;

	error_class = ERROR_GET_CLASS(errcode);
	
	if (error_class <= MPICH_ERR_LAST_CLASS)
	{
	    fprintf(fp, "(unknown)(): %s\n", get_class_msg(ERROR_GET_CLASS(errcode)));
	}
	else
	{
	    /* FIXME: Not internationalized */
	    fprintf(fp, "Error code contains an invalid class (%d)\n", error_class);
	}
    }
    
  fn_exit:
    return;
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
		MPIU_Error_printf( 
		    "Invalid error code (%d) (error ring index %d invalid)\n", 
		    errcode, ring_idx );
		break;
	    }

	    if (generic_idx < 0) {
		break;
	    }
	    
	    if (ErrorRing[ring_idx].id == ring_id) {
		len = (int)strlen(ErrorRing[ring_idx].location);
		max_location_len = MPIR_MAX(max_location_len, len);
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
		MPIU_Error_printf( 
		    "Invalid error code (%d) (error ring index %d invalid)\n", 
		    errcode, ring_idx );
	    }
	    
	    if (generic_idx < 0)
	    {
		break;
	    }
	    
	    if (ErrorRing[ring_idx].id == ring_id) {
		int nchrs;
		MPIU_Snprintf(str, maxlen, "%s", ErrorRing[ring_idx].location);
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
		
		if (MPIR_PARAM_CHOP_ERROR_STACK > 0)
		{
		    cur_pos = ErrorRing[ring_idx].msg;
		    len = (int)strlen(cur_pos);
		    if (len == 0 && maxlen > 0) {
			*str++ = '\n';
			maxlen--;
		    }
		    while (len)
		    {
			if (len >= MPIR_PARAM_CHOP_ERROR_STACK - max_location_len)
			{
			    if (len > maxlen)
				break;
			    /* FIXME: Don't use Snprint to append a string ! */
			    MPIU_Snprintf(str, MPIR_PARAM_CHOP_ERROR_STACK - 1 - max_location_len, "%s", cur_pos);
			    str[MPIR_PARAM_CHOP_ERROR_STACK - 1 - max_location_len] = '\n';
			    cur_pos += MPIR_PARAM_CHOP_ERROR_STACK - 1 - max_location_len;
			    str += MPIR_PARAM_CHOP_ERROR_STACK - max_location_len;
			    maxlen -= MPIR_PARAM_CHOP_ERROR_STACK - max_location_len;
			    if (maxlen < max_location_len)
				break;
			    for (i=0; i<max_location_len; i++)
			    {
				MPIU_Snprintf(str, maxlen, " ");
				maxlen--;
				str++;
			    }
			    len = (int)strlen(cur_pos);
			}
			else
			{
			    MPIU_Snprintf(str, maxlen, "%s\n", cur_pos);
			    len = (int)strlen(str);
			    maxlen -= len;
			    str += len;
			    len = 0;
			}
		    }
		}
		else
		{
		    MPIU_Snprintf(str, maxlen, "%s\n", ErrorRing[ring_idx].msg);
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
	    MPIU_Snprintf(str, maxlen, "(unknown)(): %s\n", p );
	    len = (int)strlen(str);
	    maxlen -= len;
	    str += len;
	    goto fn_exit;
	}
    }
    
    {
	int error_class;

	error_class = ERROR_GET_CLASS(errcode);
	
	if (error_class <= MPICH_ERR_LAST_CLASS)
	{
	    MPIU_Snprintf(str, maxlen, "(unknown)(): %s\n", 
			  get_class_msg(ERROR_GET_CLASS(errcode)));
	    len = (int)strlen(str);
	    maxlen -= len;
	    str += len;
	}
	else
	{
	    /* FIXME: Not internationalized */
	    MPIU_Snprintf(str, maxlen, 
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

static int ErrGetInstanceString( int errorcode, char *msg, int num_remaining )
{
    int len;

    if (MPIR_PARAM_PRINT_ERROR_STACK) {
	MPIU_Strncpy(msg, ", error stack:\n", num_remaining);
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
		    MPIU_Error_printf( 
	      "Invalid error code (%d) (error ring index %d invalid)\n", 
	      errorcode, ring_idx );
		    break;
		}
		
		if (generic_idx < 0) {
		    break;
		}
		
		if (ErrorRing[ring_idx].id == ring_id) {
		    /* just keep clobbering old values until the 
		       end of the stack is reached */
		    MPIU_Snprintf(msg, num_remaining, ", %s", 
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
#endif /* MSG_LEVEL = MSG_ALL */
