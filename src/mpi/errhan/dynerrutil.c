/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

#include <string.h>

/*
 * This file contains the routines needed to implement the MPI routines that
 * can add error classes and codes during runtime.  This file is organized
 * so that applications that do not use the MPI-2 routines to create new
 * error codes will not load any of this code.  
 * 
 * ROMIO has been customized to provide error messages with the same tools
 * as the rest of MPICH2 and will not rely on the dynamically assigned
 * error classes.  This leaves all of the classes and codes for the user.
 *
 * Because we have customized ROMIO, we do not need to implement 
 * instance-specific messages for the dynamic error codes.  
 */

/* Local data structures.
   A message may be associated with each class and code.
   Since we limit the number of user-defined classes and code (no more
   than 256 of each), we allocate an array of pointers to the messages here.

   We *could* allow 256 codes with each class.  However, we don't expect 
   any need for this many codes, so we simply allow 256 (actually
   ERROR_MAX_NCODE) codes, and distribute these among the error codes.

   A user-defined error code has the following format.  The ERROR_xxx
   is the macro that may be used to extract the data (usually a MASK and
   a (right)shift)

   [0-6] Class (same as predefined error classes);  ERROR_CLASS_MASK
   [7]   Is dynamic; ERROR_DYN_MASK and ERROR_DYN_SHIFT
   [8-18] Code index (for messages); ERROR_GENERIC_MASK and ERROR_GENERIC_SHIFT
   [19-31] Zero (unused but defined as zero)
*/

static int  not_initialized = 1;  /* This allows us to use atomic decr */
static const char *(user_class_msgs[ERROR_MAX_NCLASS]) = { 0 };
static const char *(user_code_msgs[ERROR_MAX_NCODE]) = { 0 };
static int  first_free_class = 0;
static int  first_free_code  = 1;  /* code 0 is reserved */

/* Forward reference */
const char *MPIR_Err_get_dynerr_string( int code );

/* This external allows this package to define the routine that converts
   dynamically assigned codes and classes to their corresponding strings. 
   A cleaner implementation could replace this exposed global with a method
   defined in the error_string.c file that allowed this package to set 
   the routine. */

static int MPIR_Dynerrcodes_finalize( void * );

/* Local routine to initialize the data structures for the dynamic
   error classes and codes.

   MPIR_Init_err_dyncodes is called if not_initialized is true.  
   Because all of the routines in this file are called by the 
   MPI_Add_error_xxx routines, and those routines use the SINGLE_CS
   when the implementation is multithreaded, these routines (until 
   we implement finer-grain thread-synchronization) need not worry about
   multiple threads
 */
static void MPIR_Init_err_dyncodes( void )
{
    int i;

    /* FIXME: Does this need a thread-safe init? */
    not_initialized = 0;
    
    for (i=0; i<ERROR_MAX_NCLASS; i++) {
	user_class_msgs[i] = 0;
    }
    for (i=0; i<ERROR_MAX_NCODE; i++) {
	user_code_msgs[i] = 0;
    }
    /* Set the routine to provides access to the dynamically created
       error strings */
    MPIR_Process.errcode_to_string = MPIR_Err_get_dynerr_string;

    /* Add a finalize handler to free any allocated space */
    MPIR_Add_finalize( MPIR_Dynerrcodes_finalize, (void*)0, 9 );
}

/*
  MPIR_Err_set_msg - Change the message for an error code or class

  Input Parameter:
+ code - Error code or class
- msg  - New message to use

  Notes:
  This routine is needed to implement 'MPI_Add_error_string'.
*/
int MPIR_Err_set_msg( int code, const char *msg_string )
{
    int errcode, errclass;
    size_t msg_len;
    char *str;
    static const char FCNAME[] = "MPIR_Err_set_msg";

    /* --BEGIN ERROR HANDLING-- */
    if (not_initialized) {
	/* Just to keep the rest of the code more robust, we'll 
	   initialize the dynamic error codes *anyway*, but this is 
	   an error (see MPI_Add_error_string in the standard) */
	MPIR_Init_err_dyncodes();
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
				     "MPIR_Err_set_msg", __LINE__, 
				     MPI_ERR_ARG, "**argerrcode", 
				     "**argerrcode %d", code );
    }
    /* --END ERROR HANDLING-- */
    
    /* Error strings are attached to a particular error code, not class.
       As a special case, if the code is 0, we use the class message */
    errclass = code & ERROR_CLASS_MASK;
    errcode  = (code & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT;

    /* --BEGIN ERROR HANDLING-- */
    if (code & ~(ERROR_CLASS_MASK | ERROR_DYN_MASK | ERROR_GENERIC_MASK)) {
	/* Check for invalid error code */
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, 
				     MPI_ERR_ARG, "**argerrcode", 
				     "**argerrcode %d", code );
    }
    /* --END ERROR HANDLING-- */

    /* --------------------------------------------------------------------- */
    msg_len = strlen( msg_string );
    str = (char *)MPIU_Malloc( msg_len + 1 );
    /* --BEGIN ERROR HANDLING-- */
    if (!str) {
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
				     FCNAME, __LINE__, MPI_ERR_OTHER, 
				     "**nomem", "**nomem %s %d", 
				     "error message string", msg_len );
    }
    /* --END ERROR HANDLING-- */

    /* --------------------------------------------------------------------- */
    MPIU_Strncpy( str, msg_string, msg_len + 1 );
    if (errcode) {
	if (errcode < first_free_code) {
	    if (user_code_msgs[errcode]) {
		MPIU_Free( (void*)(user_code_msgs[errcode]) );
	    }
	    user_code_msgs[errcode] = (const char *)str;
	}
	else {
	    /* FIXME : Unallocated error code? */
	    MPIU_Free( str );
	}
    }
    else {
	if (errclass < first_free_class) {
	    if (user_class_msgs[errclass]) {
		MPIU_Free( (void*)(user_class_msgs[errclass]) );
	    }
	    user_class_msgs[errclass] = (const char *)str;
	}
	else {
	    /* FIXME : Unallocated error code? */
	    MPIU_Free( str );
	}
    }
       
    return MPI_SUCCESS;
}

/*
  MPIR_Err_add_class - Creata a new error class

  Return value:
  An error class.  Returns -1 if no more classes are available.

  Notes:
  This is used to implement 'MPI_Add_error_class'; it may also be used by a 
  device to add device-specific error classes.  

  Predefined classes are handled directly; this routine is not used to 
  initialize the predefined MPI error classes.  This is done to reduce the
  number of steps that must be executed when starting an MPI program.

  This routine should be run within a SINGLE_CS in the multithreaded case.
*/
int MPIR_Err_add_class(void)
{
    int new_class;

    if (not_initialized)
	MPIR_Init_err_dyncodes();
	
    /* Get new class */
    new_class = first_free_class;
    ++first_free_class;

    /* --BEGIN ERROR HANDLING-- */
    if (new_class >= ERROR_MAX_NCLASS) {
	/* Fail if out of classes */
	return -1;
    }
    /* --END ERROR HANDLING-- */

    /* Note that the MPI interface always adds an error class without
       a string.  */
    user_class_msgs[new_class] = 0;

    return (new_class | ERROR_DYN_MASK);
}

/*
  MPIR_Err_add_code - Create a new error code that is associated with an 
  existing error class

  Input Parameters:
. class - Error class to which the code belongs.

  Return value:
  An error code.

  Notes:
  This is used to implement 'MPI_Add_error_code'; it may also be used by a 
  device to add device-specific error codes.  

  */
int MPIR_Err_add_code( int class )
{
    int new_code;

    /* Note that we can add codes to existing classes, so we may
       need to initialize the dynamic error routines in this function */
    if (not_initialized)
	MPIR_Init_err_dyncodes();

    /* Get the new code */
    new_code = first_free_code;
    ++first_free_code;

    /* --BEGIN ERROR HANDLING-- */
    if (new_code >= ERROR_MAX_NCODE) {
	/* Fail if out of codes */
	return -1;
    }
    /* --END ERROR HANDLING-- */

    /* Create the full error code */
    new_code = class | ERROR_DYN_MASK | (new_code << ERROR_GENERIC_SHIFT);

    /* FIXME: For robustness, we should make sure that the associated string
       is initialized to null */
    return new_code;
}

/*
  MPIR_Err_get_dynerr_string - Get the message string that corresponds to a
  dynamically created error class or code

  Input Parameter:
+ code - An error class or code.  If a code, it must have been created by 
  'MPIR_Err_create_code'.

  Return value:
  A pointer to a null-terminated text string with the corresponding error 
  message.  A null return indicates an error; usually the value of 'code' is 
  neither a valid error class or code.

  Notes:
  This routine is used to implement 'MPI_ERROR_STRING'.  It is only called
  for dynamic error codes.  
  */
const char *MPIR_Err_get_dynerr_string( int code )
{
    int errcode, errclass;
    const char *errstr = 0;

    /* Error strings are attached to a particular error code, not class.
       As a special case, if the code is 0, we use the class message */
    errclass = code & ERROR_CLASS_MASK;
    errcode  = (code & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT;

    if (code & ~(ERROR_CLASS_MASK | ERROR_DYN_MASK | ERROR_GENERIC_MASK)) {
	/* Check for invalid error code */
	return 0;
    }

    if (errcode) {
	if (errcode < first_free_code) {
	    errstr = user_code_msgs[errcode];
	}
    }
    else {
	if (errclass < first_free_class) {
	    errstr = user_class_msgs[errclass];
	}
    }
       
    return errstr;
}


static int MPIR_Dynerrcodes_finalize( void *p ATTRIBUTE((unused)) )
{
    int i;

    MPIU_UNREFERENCED_ARG(p);

    if (not_initialized == 0) {

        for (i=0; i<first_free_class; i++) {
            if (user_class_msgs[i])
                MPIU_Free((char *) user_class_msgs[i]);
        }

        for (i=0; i<first_free_code; i++) {
            if (user_code_msgs[i])
                MPIU_Free((char *) user_code_msgs[i]);
        }
    }
    return 0;
}
