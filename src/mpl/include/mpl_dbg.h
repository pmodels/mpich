/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_DBG_H_INCLUDED
#define MPL_DBG_H_INCLUDED

#include "mplconfig.h"

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

#ifdef MPL_USE_DBG_LOGGING

#define MPL_DBG_SELECTED(_class, _level)                                \
    ((_class & MPL_DBG_ActiveClasses) && MPL_DBG_##_level <= MPL_DBG_MaxLevel)

#define MPL_DBG_MSG(_class, _level, _string)                            \
    {                                                                   \
        if ((_class & MPL_DBG_ActiveClasses) && MPL_DBG_##_level <= MPL_DBG_MaxLevel) { \
            MPL_DBG_Outevent(__FILE__, __LINE__, _class, 0, "%s", _string); \
        }                                                               \
    }

#define MPL_DBG_MSG_S(_class, _level, _fmat, _string)                   \
    {                                                                   \
        if ((_class & MPL_DBG_ActiveClasses) && MPL_DBG_##_level <= MPL_DBG_MaxLevel) { \
            MPL_DBG_Outevent(__FILE__, __LINE__, _class, 1, _fmat, _string); \
        }                                                               \
    }

#define MPL_DBG_MSG_D(_class, _level, _fmat, _int)                      \
    {                                                                   \
        if ((_class & MPL_DBG_ActiveClasses) && MPL_DBG_##_level <= MPL_DBG_MaxLevel) { \
            MPL_DBG_Outevent(__FILE__, __LINE__, _class, 2, _fmat, _int); \
        }                                                               \
    }

#define MPL_DBG_MSG_P(_class, _level, _fmat, _pointer)                  \
    {                                                                   \
        if ((_class & MPL_DBG_ActiveClasses) && MPL_DBG_##_level <= MPL_DBG_MaxLevel) { \
            MPL_DBG_Outevent(__FILE__, __LINE__, _class, 3, _fmat, _pointer); \
        }                                                               \
    }

#define MPL_DBG_MAXLINE 256
#define MPL_DBG_FDEST _s,(size_t)MPL_DBG_MAXLINE
/*M
  MPL_DBG_MSG_FMT - General debugging output macro

  Notes:
  To use this macro, the third argument should be an "sprintf" - style
  argument, using MPL_DBG_FDEST as the buffer argument.  For example,
.vb
    MPL_DBG_MSG_FMT(CMM,VERBOSE,(MPL_DBG_FDEST,"fmat",args...));
.ve
  M*/

#define MPL_DBG_MSG_FMT(_class, _level, _fmatargs)                      \
    {                                                                   \
        if ((_class & MPL_DBG_ActiveClasses) && MPL_DBG_##_level <= MPL_DBG_MaxLevel) { \
            char _s[MPL_DBG_MAXLINE];                                   \
            MPL_snprintf _fmatargs ;                                    \
            MPL_DBG_Outevent(__FILE__, __LINE__, _class, 0, "%s", _s);  \
        }                                                               \
    }

#define MPL_DBG_STMT(_class, _level, _stmt)                             \
    {                                                                   \
        if ((_class & MPL_DBG_ActiveClasses) && MPL_DBG_##_level <= MPL_DBG_MaxLevel) { \
            _stmt;                                                      \
        }                                                               \
    }

#define MPL_DBG_OUT(_class, _msg)                               \
    MPL_DBG_Outevent(__FILE__, __LINE__, _class, 0, "%s", _msg)

#define MPL_DBG_OUT_FMT(_class,_fmatargs)                               \
    {                                                                   \
        char _s[MPL_DBG_MAXLINE];                                       \
        MPL_snprintf _fmatargs ;                                        \
        MPL_DBG_Outevent(__FILE__, __LINE__, _class, 0, "%s", _s);      \
    }

#else
#define MPL_DBG_SELECTED(_class,_level) 0
#define MPL_DBG_MSG(_class,_level,_string)
#define MPL_DBG_MSG_S(_class,_level,_fmat,_string)
#define MPL_DBG_MSG_D(_class,_level,_fmat,_int)
#define MPL_DBG_MSG_P(_class,_level,_fmat,_int)
#define MPL_DBG_MSG_FMT(_class,_level,_fmatargs)
#define MPL_DBG_STMT(_class,_level,_stmt)
#define MPL_DBG_OUT(_class,_msg)
#define MPL_DBG_OUT_FMT(_class,_fmtargs)
#endif

#define MPL_DBG_SUCCESS       0
#define MPL_DBG_ERR_INTERN    1
#define MPL_DBG_ERR_OTHER     2

typedef unsigned int MPL_DBG_Class;

/* Special constants */
enum MPL_DBG_LEVEL {
    MPL_DBG_TERSE = 0,
    MPL_DBG_TYPICAL = 50,
    MPL_DBG_VERBOSE = 99
};

extern int MPL_DBG_ActiveClasses;
extern int MPL_DBG_MaxLevel;

extern MPL_DBG_Class MPL_DBG_ROUTINE_ENTER;
extern MPL_DBG_Class MPL_DBG_ROUTINE_EXIT;
extern MPL_DBG_Class MPL_DBG_ROUTINE;
extern MPL_DBG_Class MPL_DBG_ALL;

MPL_DBG_Class MPL_DBG_Class_alloc(const char *ucname, const char *lcname);
void MPL_DBG_Class_register(MPL_DBG_Class class, const char *ucname, const char *lcname);

#define MPL_DBG_CLASS_CLR(class)                \
    do {                                        \
        (class) = 0;                            \
    } while (0)

#define MPL_DBG_CLASS_APPEND(out_class, in_class)       \
    do {                                                \
        (out_class) |= (in_class);                      \
    } while (0)

/* *INDENT-OFF* */
int MPL_DBG_Outevent(const char *, int, int, int, const char *, ...) ATTRIBUTE((format(printf, 5, 6)));
/* *INDENT-ON* */

int MPL_DBG_Init(int *, char ***, int, int, int, int, int);
int MPL_DBG_PreInit(int *, char ***, int);

#endif
