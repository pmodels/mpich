/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_H_INCLUDED
#define MPL_H_INCLUDED

#include "mplconfig.h"

#if defined _mpl_restrict
#define mpl_restrict _mpl_restrict
#endif /* _mpl_restrict */

#if defined _mpl_const
#define mpl_const _mpl_const
#endif /* _mpl_const */

#if defined MPL_HAVE_STDIO_H
#include <stdio.h>
#endif /* MPL_HAVE_STDIO_H */

#if defined MPL_HAVE_STDLIB_H
#include <stdlib.h>
#endif /* MPL_HAVE_STDLIB_H */

#if defined MPL_HAVE_STRING_H
#include <string.h>
#endif /* MPL_HAVE_STRING_H */

#if defined MPL_HAVE_STDARG_H
#include <stdarg.h>
#endif /* MPL_HAVE_STDARG_H */

#if defined MPL_HAVE_CTYPE_H
#include <ctype.h>
#endif /* MPL_HAVE_CTYPE_H */

#if !defined ATTRIBUTE
#  if defined MPL_HAVE_GCC_ATTRIBUTE
#    define ATTRIBUTE(a_) __attribute__(a_)
#  else /* MPL_HAVE_GCC_ATTRIBUTE */
#    define ATTRIBUTE(a_)
#  endif /* MPL_HAVE_GCC_ATTRIBUTE */
#endif /* ATTRIBUTE */

#include "mplstr.h"

#endif /* MPL_H_INCLUDED */
