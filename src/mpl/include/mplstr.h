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

#if defined MPL_NEEDS_STRDUP_DECL && !defined strdup
extern char *strdup(const char *);
#endif /* MPL_NEEDS_STRDUP_DECL */

#if defined MPL_HAVE_STRDUP
#define MPL_strdup strdup
#else
char *MPL_strdup(const char *str);
#endif /* MPL_HAVE_STRDUP */

int MPL_strncpy(char *dest, const char *src, size_t n);
char *MPL_strsep(char **stringp, const char *delim);

#if defined MPL_NEEDS_STRNCMP_DECL
extern int strncmp(const char *s1, const char *s2, size_t n);
#endif

#if defined MPL_HAVE_STRNCMP
#define MPL_strncmp strncmp
#else
#error "strncmp is required"
#endif /* MPL_HAVE_STRNCMP */

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* MPLSTR_H_INCLUDED */
