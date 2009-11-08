/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_H_INCLUDED
#define MPL_H_INCLUDED

#include "mplconfig.h"

#if defined HAVE_STDIO_H
#include <stdio.h>
#endif /* HAVE_STDIO_H */

#if defined HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if defined HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if defined HAVE_STDARG_H
#include <stdarg.h>
#endif /* HAVE_STDARG_H */

#if defined HAVE_CTYPE_H
#include <ctype.h>
#endif /* HAVE_CTYPE_H */

#if !defined ATTRIBUTE
#  if defined HAVE_GCC_ATTRIBUTE
#    define ATTRIBUTE(a_) __attribute__(a_)
#  else /* HAVE_GCC_ATTRIBUTE */
#    define ATTRIBUTE(a_)
#  endif /* HAVE_GCC_ATTRIBUTE */
#endif /* ATTRIBUTE */

#include "mplstr.h"

#endif /* MPL_H_INCLUDED */
