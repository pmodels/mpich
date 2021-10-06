/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_STR_H_INCLUDED
#define MPL_STR_H_INCLUDED

#include "mplconfig.h"

/* NOTE: PATH_MAX is simply an arbitrary convenience size. It only be used
 * in non-critical paths or where we are certain the file path is very short.
 * Critical paths should consider using dynamic buffer.
 */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* *INDENT-ON* */
#if defined(__cplusplus)
extern "C" {
#endif
/* *INDENT-OFF* */

#if defined MPL_NEEDS_SNPRINTF_DECL
extern int snprintf(char *, size_t, const char *, ...) ATTRIBUTE((format(printf,3,4)));
#endif

#define MPL_snprintf_nowarn(...)                \
    (MPL_snprintf(__VA_ARGS__) < 0 ? assert(0) : (void) 0)

#if defined MPL_HAVE_SNPRINTF
#define MPL_snprintf snprintf
#else
int MPL_snprintf(char *, size_t, const char *, ...) ATTRIBUTE((format(printf,3,4)));
#endif /* MPL_HAVE_SNPRINTF */

int MPL_strncpy(char *dest, const char *src, size_t n);
char *MPL_strsep(char **stringp, const char *delim);

#if defined MPL_NEEDS_STRERROR_DECL
extern char *strerror(int errnum);
#endif
#if defined MPL_HAVE_STRERROR
#define MPL_strerror strerror
#else
char *MPL_strerror(int errnum);
#endif /* MPL_HAVE_STRERROR */

int MPL_strnapp(char *dest, const char *src, size_t n);
void MPL_create_pathname(char *dest_filename, const char *dirname,
                         const char *prefix, const int is_dir);

int MPL_stricmp(const char *a, const char *b);

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* MPL_STR_H_INCLUDED */
