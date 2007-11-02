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
#include "mpimem.h"

/* mpibase includes definitions of the MPIU_xxx_printf routines */
#include "mpibase.h"

#endif
