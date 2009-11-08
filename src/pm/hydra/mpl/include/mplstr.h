/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPLSTR_H_INCLUDED
#define MPLSTR_H_INCLUDED

#include "mplconfig.h"

/* *INDENT-ON* */
#if defined(__cplusplus)
extern "C" {
#endif
/* *INDENT-OFF* */

#if defined NEEDS_SNPRINTF_DECL
extern int snprintf(char *, size_t, const char *, ...) ATTRIBUTE((format(printf,3,4)));
#endif

#if !defined HAVE_SNPRINTF
#define snprintf MPL_snprintf
#endif /* HAVE_SNPRINTF */

#if defined NEEDS_STRDUP_DECL
extern char *strdup(const char *);
#endif /* NEEDS_STRDUP_DECL */

#if !defined HAVE_STRDUP
#define strdup  MPL_strdup
#endif /* HAVE_STRDUP */

int MPL_snprintf(char *, size_t, const char *, ...);
char *MPL_strdup(const char *str);

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* MPLSTR_H_INCLUDED */
