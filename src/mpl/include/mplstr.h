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

/* FIXME: This is a hack to not define MPL_strdup when memory tracing
 * is enabled. This is needed because the memory tracing code is still
 * left behind in MPICH2 and not migrated to MPL; so MPICH2 undefines
 * strdup (requires the code to use MPIU_Strdup instead), while MPL
 * tries to use strdup. */
#if !defined USE_MEMORY_TRACING
#if defined MPL_NEEDS_STRDUP_DECL && !defined strdup
extern char *strdup(const char *);
#endif /* MPL_NEEDS_STRDUP_DECL */

#if defined MPL_HAVE_STRDUP
#define MPL_strdup strdup
#else
char *MPL_strdup(const char *str);
#endif /* MPL_HAVE_STRDUP */
#endif /* USE_MEMORY_TRACING */

int MPL_strncpy(char *dest, const char *src, size_t n);

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* MPLSTR_H_INCLUDED */
