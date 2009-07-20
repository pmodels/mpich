/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpichconf.h"
#include "mpimem.h"
#include "mpibase.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#ifdef HAVE_STDARG_H
    #include <stdarg.h>
#endif

#if defined(USE_GETTEXT)
#include <libintl.h>
#endif

/* style: allow:vprintf:1 sig:0 */
/* style: allow:vfprintf:4 sig:0 */
/* style: allow:fprintf:2 sig:0 */

int MPIU_Usage_printf(const char *str, ...)
{
    int n;
    va_list list;
    const char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vprintf(format_str, list);
    va_end(list);

    fflush(stdout);

    return n;
}

int MPIU_Error_printf(const char *str, ...)
{
    int n;
    va_list list;
    const char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vfprintf(stderr, format_str, list);
    va_end(list);

    fflush(stderr);

    return n;
}

int MPIU_Internal_error_printf(const char *str, ...)
{
    int n;
    va_list list;
    const char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vfprintf(stderr, format_str, list);
    va_end(list);

    fflush(stderr);

    return n;
}

/* Like internal_error_printf, but for the system routine name with 
   errno errnum.  Str may be null */
int MPIU_Internal_sys_error_printf(const char *name, int errnum, 
				   const char *str, ...)
{
    int n = 0;
    va_list list;
    const char *format_str=0;

    /* Prepend information on the system error */
#ifdef HAVE_STRERROR
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", "Error in system call %s: %s" );
#endif
    if (!format_str) format_str = "Error in system call %s: %s\n";

    fprintf( stderr, format_str, name, strerror(errnum) );
#else
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", "Error in system call %s errno = %d" );
#endif
    if (!format_str) format_str = "Error in system call %s errno = %d\n";

    fprintf( stderr, "Error in %s: errno = %d\n", name, errnum );
#endif

    /* Now add the message that is specific to this use, if any */
    if (str) {
	va_start(list, str);
#ifdef USE_GETTEXT
	/* Category is LC_MESSAGES */
	format_str = dgettext( "mpich", str );
	if (!format_str) format_str = str;
#else
	format_str = str;
#endif
	n = vfprintf(stderr, format_str, list);
	va_end(list);
    }

    fflush(stderr);

    return n;
}

int MPIU_Msg_printf(const char *str, ...)
{
    int n;
    va_list list;
    const char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vfprintf(stdout, format_str, list);
    va_end(list);

    fflush(stdout);

    return n;
}

