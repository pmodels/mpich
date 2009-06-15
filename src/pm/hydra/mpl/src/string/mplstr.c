/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mplstr.h"

#ifndef HAVE_SNPRINTF
/* FIXME: This is an approximate form, which works for most cases, but
 * might not work for all. */
int MPL_snprintf(char *str, size_t size, const char *format, ...)
{
    int n;
    const char *p;
    char *out_str = str;
    va_list list;

    va_start(list, format);

    p = format;
    while (*p && size > 0) {
        char *nf;

        nf = strchr(p, '%');
        if (!nf) {
            /* No more format characters */
            while (size-- > 0 && *p) {
                *out_str++ = *p++;
            }
        }
        else {
            int nc;
            int width = -1;

            /* Copy until nf */
            while (p < nf && size-- > 0) {
                *out_str++ = *p++;
            }
            /* p now points at nf */
            /* Handle the format character */
            nc = nf[1];
            if (isdigit(nc)) {
                /* Get the field width */
                /* FIXME : Assumes ASCII */
                width = nc - '0';
                p = nf + 2;
                while (*p && isdigit(*p)) {
                    width = 10 * width + (*p++ - '0');
                }
                /* When there is no longer a digit, get the format
                 * character */
                nc = *p++;
            }
            else {
                /* Skip over the format string */
                p += 2;
            }

            switch (nc) {
            case '%':
                *out_str++ = '%';
                size--;
                break;

            case 'd':
                {
                    int val;
                    char tmp[20];
                    char *t = tmp;
                    /* Get the argument, of integer type */
                    val = va_arg(list, int);
                    sprintf(tmp, "%d", val);
                    if (width > 0) {
                        int tmplen = strlen(tmp);
                        /* If a width was specified, pad with spaces on the
                         * left (on the right if %-3d given; not implemented yet */
                        while (size-- > 0 && width-- > tmplen)
                            *out_str++ = ' ';
                    }
                    while (size-- > 0 && *t) {
                        *out_str++ = *t++;
                    }
                }
                break;

            case 'x':
                {
                    int val;
                    char tmp[20];
                    char *t = tmp;
                    /* Get the argument, of integer type */
                    val = va_arg(list, int);
                    sprintf(tmp, "%x", val);
                    if (width > 0) {
                        int tmplen = strlen(tmp);
                        /* If a width was specified, pad with spaces on the
                         * left (on the right if %-3d given; not implemented yet */
                        while (size-- > 0 && width-- > tmplen)
                            *out_str++ = ' ';
                    }
                    while (size-- > 0 && *t) {
                        *out_str++ = *t++;
                    }
                }
                break;

            case 'p':
                {
                    void *val;
                    char tmp[20];
                    char *t = tmp;
                    /* Get the argument, of pointer type */
                    val = va_arg(list, void *);
                    sprintf(tmp, "%p", val);
                    if (width > 0) {
                        int tmplen = strlen(tmp);
                        /* If a width was specified, pad with spaces on the
                         * left (on the right if %-3d given; not implemented yet */
                        while (size-- > 0 && width-- > tmplen)
                            *out_str++ = ' ';
                    }
                    while (size-- > 0 && *t) {
                        *out_str++ = *t++;
                    }
                }
                break;

            case 's':
                {
                    char *s_arg;
                    /* Get the argument, of pointer to char type */
                    s_arg = va_arg(list, char *);
                    while (size-- > 0 && s_arg && *s_arg) {
                        *out_str++ = *s_arg++;
                    }
                }
                break;

            default:
                /* Error, unknown case */
                return -1;
                break;
            }
        }
    }

    va_end(list);

    if (size-- > 0)
        *out_str++ = '\0';

    n = (int) (out_str - str);
    return n;
}
#endif


#ifndef HAVE_STRDUP
#ifdef MPL_strdup
#undef MPL_strdup
#endif
/*@
  MPL_strdup - Duplicate a string

  Synopsis:
.vb
    char *MPL_strdup(const char *str)
.ve

  Input Parameter:
. str - null-terminated string to duplicate

  Return value:
  A pointer to a copy of the string, including the terminating null.  A
  null pointer is returned on error, such as out-of-memory.

  Module:
  Utility
  @*/
char *MPL_strdup(const char *str)
{
    char *restrict p = (char *) malloc(strlen(str) + 1);
    const char *restrict in_p = str;
    char *save_p;

    save_p = p;
    if (p) {
        while (*in_p) {
            *p++ = *in_p++;
        }
        *p = 0;
    }
    return save_p;
}
#endif
