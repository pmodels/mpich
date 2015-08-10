/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPLMSG_H_INCLUDED
#define MPLMSG_H_INCLUDED

#include "mpl.h"

/* These macros can be used to prevent inlining for utility functions
 * where it might make debugging easier. */
#if defined(HAVE_ERROR_CHECKING)
#define MPL_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((__noinline__))
#define MPL_DBG_INLINE_KEYWORD
#else
#define MPL_DBG_ATTRIBUTE_NOINLINE
#define MPL_DBG_INLINE_KEYWORD inline
#endif

/* These routines are used to ensure that messages are sent to the
 * appropriate output and (eventually) are properly
 * internationalized */
int MPL_usage_printf(mpl_const char *str, ...) ATTRIBUTE((format(printf, 1, 2)));
int MPL_msg_printf(mpl_const char *str, ...) ATTRIBUTE((format(printf, 1, 2)));
int MPL_internal_error_printf(mpl_const char *str, ...) ATTRIBUTE((format(printf, 1, 2)));
int MPL_internal_sys_error_printf(mpl_const char *, int, mpl_const char *str,
                                  ...) ATTRIBUTE((format(printf, 3, 4)));
void MPL_exit(int);

#endif /* MPLMSG_H_INCLUDED */
