/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef PMUTIL_H_INCLUDED
#define PMUTIL_H_INCLUDED

/* Allow the printf commands in the debugging statements */
/* style: allow:printf:4 sig:0 */
/* style: allow:fprintf:1 sig:0 */
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

/* 
   ---------------------------------------------------------------------------
   Function prototypes 
   ---------------------------------------------------------------------------
 */
void MPIE_CreateNewSession( void );

/* 
   ---------------------------------------------------------------------------
   Miscellaneous
   ---------------------------------------------------------------------------
 */

/* Debug value */
extern int MPIE_Debug;

/* For mpiexec, we should normally enable debugging.  Comment out this
   definition of HAVE_DEBUGGING if you want to leave these out of the code */
#define HAVE_DEBUGGING

#ifdef HAVE_DEBUGGING
#define DBG_COND(cond,stmt) {if (cond) { stmt;}}
#else
#define DBG_COND(cond,a)
#endif
#define DBG(stmt) DBG_COND(MPIE_Debug,stmt)
#define DBG_PRINTFCOND(cond,a)  DBG_COND(cond,printf a ; fflush(stdout))
#define DBG_EPRINTFCOND(cond,a) DBG_COND(cond,fprintf a ; fflush(stderr))
#define DBG_EPRINTF(a) DBG_EPRINTFCOND(MPIE_Debug,a)
#define DBG_PRINTF(a)  DBG_PRINTFCOND(MPIE_Debug,a)

/* #define USE_LOG_SYSCALLS */
#ifdef USE_LOG_SYSCALLS
#include <errno.h>
#define MPIE_SYSCALL(a_,b_,c_) { \
    printf( "about to call %s (%s,%d)\n", #b_ ,__FILE__, __LINE__);\
          fflush(stdout); errno = 0;\
    a_ = b_ c_; \
    if ((a_)>=0 || errno==0) {\
    printf( "%s returned %d\n", #b_, a_ );\
    } \
 else { \
    printf( "%s returned %d (errno = %d,%s)\n", \
          #b_, a_, errno, strerror(errno));\
    };           fflush(stdout);}
#else
#define MPIE_SYSCALL(a_,b_,c_) a_ = b_ c_
#endif


/* Use the memory defintions from mpich2/src/include */
/* #include "mpimem.h" */
/* The memory routines no longer are available as utility routines.
   The choices are to use the original memory tracing routines or
   to select the option of using the basic memory routines.  The 
   second option is used for now. */
/* No memory tracing; just use native functions */
#include <stdlib.h>
#define MPIU_Malloc(a)    malloc((size_t)(a))
#define MPIU_Calloc(a,b)  calloc((size_t)(a),(size_t)(b))
#define MPIU_Free(a)      free((void *)(a))
#define MPIU_Realloc(a,b)  realloc((void *)(a),(size_t)(b))

int MPIU_Strncpy( char *outstr, const char *instr, size_t maxlen );
int MPIU_Strnapp( char *, const char *, size_t );
char *MPIU_Strdup( const char * );

#ifdef HAVE_STRDUP
/* Watch for the case where strdup is defined as a macro by a header include */
# if defined(NEEDS_STRDUP_DECL) && !defined(strdup)
extern char *strdup( const char * );
# endif
#define MPIU_Strdup(a)    strdup(a)
#else
/* Don't define MPIU_Strdup, provide it in safestr.c */
#endif /* HAVE_STRDUP */
/* Provide a fallback snprintf for systems that do not have one */
#ifdef HAVE_SNPRINTF
#define MPIU_Snprintf snprintf
/* Sometimes systems don't provide prototypes for snprintf */
#ifdef NEEDS_SNPRINTF_DECL
extern int snprintf( char *, size_t, const char *, ... ) ATTRIBUTE((format(printf,3,4)));
#endif
#else
int MPIU_Snprintf( char *str, size_t size, const char *format, ... ) 
     ATTRIBUTE((format(printf,3,4)));
#endif /* HAVE_SNPRINTF */


/* mpibase includes definitions of the MPIU_xxx_printf routines */
#include "mpibase.h"

#endif
