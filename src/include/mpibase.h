/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIBASE_H_INCLUDED
#define MPIBASE_H_INCLUDED

/* This file contains various basis definitions that may be used 
   by other source files (in particular, ones in src/util) to access
   debugging and other standard features 

   An appropriate "conf" file produced by configure should be defined before 
   this file is loaded.  The following CPP definitions are recognized:

   HAVE_GCC_ATTRIBUTE - supports the GCC __attribute__(...) extension
*/

/* 
   If we do not have the GCC attribute, then make this empty.  We use
   the GCC attribute to improve error checking by the compiler, particularly 
   for printf/sprintf strings 
*/
#ifndef ATTRIBUTE
#ifdef HAVE_GCC_ATTRIBUTE
#define ATTRIBUTE(a_) __attribute__(a_)
#else
#define ATTRIBUTE(a_)
#endif
#endif

/* These macros can be used to prevent inlining for utility functions
 * where it might make debugging easier. */
#if defined(HAVE_ERROR_CHECKING)
#define MPIU_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((__noinline__))
#define MPIU_DBG_INLINE_KEYWORD
#else
#define MPIU_DBG_ATTRIBUTE_NOINLINE
#define MPIU_DBG_INLINE_KEYWORD inline
#endif

/* These routines are used to ensure that messages are sent to the
   appropriate output and (eventually) are properly internationalized */
int MPIU_Usage_printf( const char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Msg_printf( const char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Error_printf( const char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Internal_error_printf( const char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Internal_sys_error_printf( const char *, int, const char *str, ... ) ATTRIBUTE((format(printf,3,4)));
void MPIU_Exit( int );

#endif
