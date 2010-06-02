/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIERROR_H_INCLUDED
#define MPIERROR_H_INCLUDED

/* Error severity */
#define MPIR_ERR_FATAL 1
#define MPIR_ERR_RECOVERABLE 0

struct MPID_Comm;
struct MPID_Win;
/*struct MPID_File;*/

/* Bindings for internal routines */
int MPIR_Err_return_comm( struct MPID_Comm *, const char [], int );
int MPIR_Err_return_win( struct MPID_Win *, const char [], int );
/*int MPIR_Err_return_file( struct MPID_File *, const char [], int );*/
#ifdef MPI__FILE_DEFINED
/* Only define if we have MPI_File */
int MPIR_Err_return_file( MPI_File, const char [], int ); /* Romio version */
#endif
/* FIXME:
 * Update this description to match the current version of the routine, 
 * in particular, the pseudo-format types (even better, fix it so that
 * the pseudo format types can work with the format attribute check).
 */
/*@
  MPIR_Err_create_code - Create an error code and associated message
  to report an error

  Input Parameters:
+ lastcode - Previous error code (see notes)
. severity  - Indicates severity of error
. fcname - Name of the function in which the error has occurred.  
. line  - Line number (usually '__LINE__')
. class - Error class
. generic_msg - A generic message to be used if not instance-specific
 message is available
. instance_msg - A message containing printf-style formatting commands
  that, when combined with the instance_parameters, specify an error
  message containing instance-specific data.
- instance_parameters - The remaining parameters.  These must match
 the formatting commands in 'instance_msg'.

 Notes:
 A typical use is\:
.vb
   mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE, 
               FCNAME, __LINE__, MPI_ERR_RANK, 
               "Invalid Rank", "Invalid rank %d", rank );
.ve
 
  Predefined message may also be used.  Any message that uses the
  prefix '"**"' will be looked up in a table.  This allows standardized 
  messages to be used for a message that is used in several different locations
  in the code.  For example, the name '"**rank"' might be used instead of
  '"Invalid Rank"'; this would also allow the message to be made more
  specific and useful, such as 
.vb
   Invalid rank provided.  The rank must be between 0 and the 1 less than
   the size of the communicator in this call.
.ve
  This interface is compatible with the 'gettext' interface for 
  internationalization, in the sense that the 'generic_msg' and 'instance_msg' 
  may be used as arguments to 'gettext' to return a string in the appropriate 
  language; the implementation of 'MPID_Err_create_code' can then convert
  this text into the appropriate code value.

  The current set of formatting commands is undocumented and will change.
  You may safely use '%d' and '%s' (though only use '%s' for names of 
  objects, not text messages, as using '%s' for a message breaks support for
  internationalization.

  This interface allows error messages to be chained together.  The first 
  argument is the last error code; if there is no previous error code, 
  use 'MPI_SUCCESS'.  

  Extended Format Specifiers:
  In addition to the standard format specifies (e.g., %d for an int value),
  MPIR_Err_create_code accepts some additional values that correspond to 
  various MPI types:
+ i - an MPI rank; recognizes 'MPI_ANY_SOURCE', 'MPI_PROC_NULL', and 
      'MPI_ROOT'
. t - an MPI tag; recognizes 'MPI_ANY_TAG'.
. A - an MPI assert value.
. C - an MPI communicator.
. D - an MPI datatype.
. E - an MPI Errhandler.
. F - an MPI File object.
. G - an MPI Group.
. I - an MPI info object.
. O - an MPI Op.
. R - an MPI Request.
- W - an MPI Window object.


  Module:
  Error

  @*/
int MPIR_Err_create_code( int, int, const char [], int, int, const char [], const char [], ... );

#ifdef USE_ERR_CODE_VALIST
int MPIR_Err_create_code_valist( int, int, const char [], int, int, const char [], const char [], va_list );
#endif

/*@
  MPIR_Err_combine_codes - Combine two error codes, or more importantly
  two lists of error messages.  The list associated with the second error
  code is appended to the list associated with the first error code.  If
  the list associated with the first error code has a dangling tail, which
  is possible if the ring has wrapped and overwritten entries that were
  once part of the list, then the append operation is not performed and
  the error code for the first list is returned.

  Input Parameter:
+ errorcode1 - the error code associated with the first list
- errorcode2 - the error code associated with the second list

  Return value:
  An error code which resolves to the combined list of error messages

  Notes:
  If errorcode1 is equal to MPI_SUCCESS, then errorcode2 is returned.
  Likewise, if errorcode2 is equal to MPI_SUCCESS, then errorcode1 is
  returned.

  Module:
  Error 
  @*/
int MPIR_Err_combine_codes(int, int);

int MPIR_Err_is_fatal(int);
void MPIR_Err_init(void);
void MPIR_Err_preOrPostInit( void );

/* FIXME: This comment is incorrect because the routine was improperly modified
   to take an additional argument (the MPIR_Err_get_class_string_func_t).  
   That arg needs to be removed and this function restored. */
/*@
  MPID_Err_get_string - Get the message string that corresponds to an error
  class or code

  Input Parameter:
+ code - An error class or code.  If a code, it must have been created by 
  'MPID_Err_create_code'.
- msg_len - Length of 'msg'.

  Output Parameter:
. msg - A null-terminated text string of length (including the null) of no
  more than 'msg_len'.  

  Return value:
  Zero on success.  Non-zero returns indicate either (a) 'msg_len' is too
  small for the message or (b) the value of 'code' is neither a valid 
  error class or code.

  Notes:
  This routine is used to implement 'MPI_ERROR_STRING'.

  Module:
  Error 

  Question:
  What values should be used for the error returns?  Should they be
  valid error codes?

  How do we get a good value for 'MPI_MAX_ERROR_STRING' for 'mpi.h'?
  See 'errgetmsg' for one idea.

  @*/
typedef int (* MPIR_Err_get_class_string_func_t)(int error, char *str, int length);
void MPIR_Err_get_string( int, char *, int, MPIR_Err_get_class_string_func_t );

int MPIR_Err_set_msg( int code, const char *msg_string );

#define MPIR_ERR_CLASS_MASK 0x0000007f
#define MPIR_ERR_CLASS_SIZE 128
#define MPIR_ERR_GET_CLASS(mpi_errno_) (mpi_errno_ & MPIR_ERR_CLASS_MASK)

#endif
