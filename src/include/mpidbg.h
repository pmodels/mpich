/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDBG_H_INCLUDED
#define MPIDBG_H_INCLUDED
#include <stdio.h>
#include <stdarg.h>
#include "mpibase.h"
/*
 * Multilevel debugging and tracing macros.
 * The design is discussed at 
 * http://www-unix.mcs.anl.gov/mpi/mpich2/developer/design/debugmsg.htm
 *
 * Basically, this provide a way to place debugging messages into
 * groups (called *classes*), with levels of detail, and arbitrary
 * messages.  The messages can be turned on and off using environment
 * variables and/or command-line options.
 */

#ifdef USE_DBG_LOGGING
#define MPIU_DBG_SELECTED(_class,_level) \
   ((MPIU_DBG_##_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel)
#define MPIU_DBG_MSG(_class,_level,_string)  \
   {if ( (MPIU_DBG_##_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, MPIU_DBG_##_class, 0, "%s", _string ); }}
#define MPIU_DBG_MSG_S(_class,_level,_fmat,_string) \
   {if ( (MPIU_DBG_##_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, MPIU_DBG_##_class, 1, _fmat, _string ); }}
#define MPIU_DBG_MSG_D(_class,_level,_fmat,_int) \
   {if ( (MPIU_DBG_##_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, MPIU_DBG_##_class, 2, _fmat, _int ); }}
#define MPIU_DBG_MSG_P(_class,_level,_fmat,_pointer) \
   {if ( (MPIU_DBG_##_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, MPIU_DBG_##_class, 3, _fmat, _pointer ); }}

#define MPIU_DBG_MAXLINE 256
#define MPIU_DBG_FDEST _s,(size_t)MPIU_DBG_MAXLINE
/*M
  MPIU_DBG_MSG_FMT - General debugging output macro

  Notes:
  To use this macro, the third argument should be an "sprintf" - style 
  argument, using MPIU_DBG_FDEST as the buffer argument.  For example,
.vb
    MPIU_DBG_MSG_FMT(CMM,VERBOSE,(MPIU_DBG_FDEST,"fmat",args...));
.ve  
  M*/
#define MPIU_DBG_MSG_FMT(_class,_level,_fmatargs) \
   {if ( (MPIU_DBG_##_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
          char _s[MPIU_DBG_MAXLINE]; \
          MPIU_Snprintf _fmatargs ; \
     MPIU_DBG_Outevent( __FILE__, __LINE__, MPIU_DBG_##_class, 0, "%s", _s ); }}
#define MPIU_DBG_STMT(_class,_level,_stmt) \
   {if ( (MPIU_DBG_##_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) { _stmt; }}

#define MPIU_DBG_OUT(_class,_msg) \
    MPIU_DBG_Outevent( __FILE__, __LINE__, MPIU_DBG_##_class, 0, "%s", _msg )
#define MPIU_DBG_OUT_FMT(_class,_fmatargs) \
    {     char _s[MPIU_DBG_MAXLINE]; \
          MPIU_Snprintf _fmatargs ; \
    MPIU_DBG_Outevent( __FILE__, __LINE__, MPIU_DBG_##_class, 0, "%s", _s );}

#else
#define MPIU_DBG_SELECTED(_class,_level) 0
#define MPIU_DBG_MSG(_class,_level,_string) 
#define MPIU_DBG_MSG_S(_class,_level,_fmat,_string)
#define MPIU_DBG_MSG_D(_class,_level,_fmat,_int)
#define MPIU_DBG_MSG_P(_class,_level,_fmat,_int)
#define MPIU_DBG_MSG_FMT(_class,_level,_fmatargs)
#define MPIU_DBG_STMT(_class,_level,_stmt)
#define MPIU_DBG_OUT(_class,_msg)
#define MPIU_DBG_OUT_FMT(_class,_fmtargs)
#endif

/* Special constants */
enum MPIU_DBG_LEVEL { MPIU_DBG_TERSE   = 0, 
		      MPIU_DBG_TYPICAL = 50,
		      MPIU_DBG_VERBOSE = 99 };
/* Any change in MPIU_DBG_CLASS must be matched by changes in 
   MPIU_Classnames in src/util/dbg/dbg_printf.c */
enum MPIU_DBG_CLASS { MPIU_DBG_PT2PT         = 0x1,
		      MPIU_DBG_RMA           = 0x2,
		      MPIU_DBG_THREAD        = 0x4,
		      MPIU_DBG_PM            = 0x8,
		      MPIU_DBG_ROUTINE_ENTER = 0x10,
		      MPIU_DBG_ROUTINE_EXIT  = 0x20,
		      MPIU_DBG_SYSCALL       = 0x40,
		      MPIU_DBG_DATATYPE      = 0x80,
		      MPIU_DBG_HANDLE        = 0x100,
		      MPIU_DBG_COMM          = 0x200,
		      MPIU_DBG_BSEND         = 0x400,
		      MPIU_DBG_OTHER         = 0x800,
		      MPIU_DBG_CH3_CONNECT   = 0x1000,
		      MPIU_DBG_CH3_DISCONNECT= 0x2000,
		      MPIU_DBG_CH3_PROGRESS  = 0x4000,
		      MPIU_DBG_CH3_CHANNEL   = 0x8000,
		      MPIU_DBG_CH3_OTHER     = 0x10000,
		      MPIU_DBG_CH3_MSG       = 0x20000,
		      MPIU_DBG_CH3           = 0x3f000, /* alias for all Ch3*/
                      MPIU_DBG_NEM_SOCK_FUNC = 0x40000,
                      MPIU_DBG_NEM_SOCK_DET  = 0x80000,
		      MPIU_DBG_VC            = 0x100000,
		      MPIU_DBG_REFCOUNT      = 0x200000,
		      MPIU_DBG_ROMIO         = 0x400000,
                      MPIU_DBG_ERRHAND       = 0x800000,
		      MPIU_DBG_ALL           = (~0) };   /* alias for all */

extern int MPIU_DBG_ActiveClasses;
extern int MPIU_DBG_MaxLevel;
typedef enum MPIU_dbg_state_t
{
    MPIU_DBG_STATE_NONE = 0,
    MPIU_DBG_STATE_UNINIT = 1,
    MPIU_DBG_STATE_STDOUT = 2,
    MPIU_DBG_STATE_MEMLOG = 4,
    MPIU_DBG_STATE_FILE = 8
}
MPIU_dbg_state_t;

int MPIU_dbg_init(int rank);
int MPIU_dbg_printf(const char *str, ...) ATTRIBUTE((format(printf,1,2)));
int MPIU_dbglog_printf(const char *str, ...) ATTRIBUTE((format(printf,1,2)));
int MPIU_dbglog_vprintf(const char *str, va_list ap);
void MPIU_dump_dbg_memlog_to_stdout(void);
void MPIU_dump_dbg_memlog_to_file(const char *filename);
void MPIU_dump_dbg_memlog(FILE * fp);

extern MPIU_dbg_state_t MPIU_dbg_state;
extern FILE * MPIU_dbg_fp;
#define MPIU_dbglog_flush()				\
{							\
    if (MPIU_dbg_state & MPIU_DBG_STATE_STDOUT)	\
    {							\
	fflush(stdout);					\
    }							\
}
int MPIU_DBG_Outevent(const char *, int, int, int, const char *, ...) 
                                        ATTRIBUTE((format(printf,5,6)));
int MPIU_DBG_Init( int *, char ***, int, int, int );
int MPIU_DBG_PreInit( int *, char ***, int );

#endif
