/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDBG_H_INCLUDED
#define MPIDBG_H_INCLUDED
#include <stdio.h>
#include <stdarg.h>
#include "mpl.h"

/*
 * Multilevel debugging and tracing macros.
 * The design is discussed at
 * http://wiki.mpich.org/mpich/index.php/Debug_Event_Logging
 *
 * Basically, this provide a way to place debugging messages into
 * groups (called *classes*), with levels of detail, and arbitrary
 * messages.  The messages can be turned on and off using environment
 * variables and/or command-line options.
 */

#ifdef USE_DBG_LOGGING
#define MPIU_DBG_SELECTED(_class,_level) \
   ((_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel)
#define MPIU_DBG_MSG(_class,_level,_string)  \
   {if ( (_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, _class, 0, "%s", _string ); }}
#define MPIU_DBG_MSG_S(_class,_level,_fmat,_string) \
   {if ( (_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, _class, 1, _fmat, _string ); }}
#define MPIU_DBG_MSG_D(_class,_level,_fmat,_int) \
   {if ( (_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, _class, 2, _fmat, _int ); }}
#define MPIU_DBG_MSG_P(_class,_level,_fmat,_pointer) \
   {if ( (_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
     MPIU_DBG_Outevent( __FILE__, __LINE__, _class, 3, _fmat, _pointer ); }}

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
   {if ( (_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) {\
          char _s[MPIU_DBG_MAXLINE]; \
          MPL_snprintf _fmatargs ; \
     MPIU_DBG_Outevent( __FILE__, __LINE__, _class, 0, "%s", _s ); }}
#define MPIU_DBG_STMT(_class,_level,_stmt) \
   {if ( (_class & MPIU_DBG_ActiveClasses) && \
          MPIU_DBG_##_level <= MPIU_DBG_MaxLevel ) { _stmt; }}

#define MPIU_DBG_OUT(_class,_msg) \
    MPIU_DBG_Outevent( __FILE__, __LINE__, _class, 0, "%s", _msg )
#define MPIU_DBG_OUT_FMT(_class,_fmatargs) \
    {     char _s[MPIU_DBG_MAXLINE]; \
          MPL_snprintf _fmatargs ; \
    MPIU_DBG_Outevent( __FILE__, __LINE__, _class, 0, "%s", _s );}

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

typedef unsigned int MPIU_DBG_Class;

/* Special constants */
enum MPIU_DBG_LEVEL { MPIU_DBG_TERSE   = 0, 
		      MPIU_DBG_TYPICAL = 50,
		      MPIU_DBG_VERBOSE = 99 };

extern int MPIU_DBG_ActiveClasses;
extern int MPIU_DBG_MaxLevel;

extern MPIU_DBG_Class MPIU_DBG_ROUTINE_ENTER;
extern MPIU_DBG_Class MPIU_DBG_ROUTINE_EXIT;
extern MPIU_DBG_Class MPIU_DBG_ROUTINE;
extern MPIU_DBG_Class MPIU_DBG_ALL;

MPIU_DBG_Class MPIU_DBG_Class_alloc(const char *ucname, const char *lcname);
void MPIU_DBG_Class_register(MPIU_DBG_Class class, const char *ucname, const char *lcname);

#define MPIU_DBG_CLASS_CLR(class)               \
    do {                                        \
        (class) = 0;                            \
    } while (0)
#define MPIU_DBG_CLASS_APPEND(out_class, in_class)      \
    do {                                                \
        (out_class) |= (in_class);                      \
    } while (0)

int MPIU_DBG_Outevent(const char *, int, int, int, const char *, ...) 
                                        ATTRIBUTE((format(printf,5,6)));
int MPIU_DBG_Init( int *, char ***, int, int, int, int );
int MPIU_DBG_PreInit( int *, char ***, int );

#endif
