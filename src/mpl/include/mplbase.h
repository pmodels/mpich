/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPLBASE_H_INCLUDED
#define MPLBASE_H_INCLUDED

#include "mplconfig.h"

#if defined MPL_HAVE_STDIO_H
#include <stdio.h>
#endif /* MPL_HAVE_STDIO_H */

#if defined MPL_HAVE_STDLIB_H
#include <stdlib.h>
#endif /* MPL_HAVE_STDLIB_H */

#if defined MPL_HAVE_STRING_H
#include <string.h>
#endif /* MPL_HAVE_STRING_H */

#if defined MPL_HAVE_MATH_H
#include <math.h>
#endif /* MPL_HAVE_MATH_H */

#if defined MPL_HAVE_STDARG_H
#include <stdarg.h>
#endif /* MPL_HAVE_STDARGS_H */

#ifndef MPL_ATTRIBUTE
#ifdef MPL_HAVE_GCC_ATTRIBUTE
#define MPL_ATTRIBUTE(a_) __attribute__(a_)
#else /* HAVE_GCC_ATTRIBUTE */
#define MPL_ATTRIBUTE(a_)
#endif /* HAVE_GCC_ATTRIBUTE */
#endif /* MPL_ATTRIBUTE */

#endif /* MPLBASE_H_INCLUDED */
