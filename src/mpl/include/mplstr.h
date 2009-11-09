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

#if defined MPL_NEEDS_SNPRINTF_DECL
extern int snprintf(char *, size_t, const char *, ...) ATTRIBUTE((format(printf,3,4)));
#endif

#if defined MPL_HAVE_SNPRINTF
#define MPL_snprintf snprintf
#else
int MPL_snprintf(char *, size_t, const char *, ...);
#endif /* MPL_HAVE_SNPRINTF */

#if defined MPL_NEEDS_STRDUP_DECL
extern char *strdup(const char *);
#endif /* MPL_NEEDS_STRDUP_DECL */

#if defined MPL_HAVE_STRDUP
#define MPL_strdup strdup
#else
char *MPL_strdup(const char *str);
#endif /* MPL_HAVE_STRDUP */

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* MPLSTR_H_INCLUDED */
