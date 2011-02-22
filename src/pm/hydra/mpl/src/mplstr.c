/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

#if !defined MPL_HAVE_SNPRINTF
int MPL_snprintf(char *str, size_t size, mpl_const char *format, ...)
{
    int n;
    mpl_const char *p;
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
#endif /* MPL_HAVE_SNPRINTF */

/*@
  MPL_strdup - Duplicate a string

  Synopsis:
.vb
    char *MPL_strdup(mpl_const char *str)
.ve

  Input Parameter:
. str - null-terminated string to duplicate

  Return value:
  A pointer to a copy of the string, including the terminating null.  A
  null pointer is returned on error, such as out-of-memory.

  Module:
  Utility
  @*/
#if !defined MPL_HAVE_STRDUP
char *MPL_strdup(mpl_const char *str)
{
    char *mpl_restrict p = (char *) malloc(strlen(str) + 1);
    mpl_const char *mpl_restrict in_p = str;
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
#endif /* MPL_HAVE_STRDUP */

/*
 * MPL_strncpy - Copy at most n characters.  Stop once a null is reached.
 *
 * This is different from strncpy, which null pads so that exactly
 * n characters are copied.  The strncpy behavior is correct for many
 * applications because it guarantees that the string has no uninitialized
 * data.
 *
 * If n characters are copied without reaching a null, return an error.
 * Otherwise, return 0.
 *
 * Question: should we provide a way to request the length of the string,
 * since we know it?
 */
/*@ MPL_strncpy - Copy a string with a maximum length

    Input Parameters:
+   instr - String to copy
-   maxlen - Maximum total length of 'outstr'

    Output Parameter:
.   outstr - String to copy into

    Notes:
    This routine is the routine that you wish 'strncpy' was.  In copying
    'instr' to 'outstr', it stops when either the end of 'outstr' (the
    null character) is seen or the maximum length 'maxlen' is reached.
    Unlike 'strncpy', it does not add enough nulls to 'outstr' after
    copying 'instr' in order to move precisely 'maxlen' characters.
    Thus, this routine may be used anywhere 'strcpy' is used, without any
    performance cost related to large values of 'maxlen'.

    If there is insufficient space in the destination, the destination is
    still null-terminated, to avoid potential failures in routines that neglect
    to check the error code return from this routine.

  Module:
  Utility
  @*/
int MPL_strncpy(char *dest, const char *src, size_t n)
{
    char *mpl_restrict d_ptr = dest;
    const char *mpl_restrict s_ptr = src;
    register int i;

    if (n == 0)
        return 0;

    i = (int) n;
    while (*s_ptr && i-- > 0) {
        *d_ptr++ = *s_ptr++;
    }

    if (i > 0) {
        *d_ptr = 0;
        return 0;
    }
    else {
        /* Force a null at the end of the string (gives better safety
         * in case the user fails to check the error code) */
        dest[n - 1] = 0;
        /* We may want to force an error message here, at least in the
         * debugging version */
        /*printf("failure in copying %s with length %d\n", src, n); */
        return 1;
    }
}

/* replacement for strsep.  Conforms to the following description (from the OS X
 * 10.6 man page):
 *
 *   The strsep() function locates, in the string referenced by *stringp, the first occur-
 *   rence of any character in the string delim (or the terminating `\0' character) and
 *   replaces it with a `\0'.  The location of the next character after the delimiter
 *   character (or NULL, if the end of the string was reached) is stored in *stringp.  The
 *   original value of *stringp is returned.
 *
 *   An ``empty'' field (i.e., a character in the string delim occurs as the first charac-
 *   ter of *stringp) can be detected by comparing the location referenced by the returned
 *   pointer to `\0'.
 *
 *   If *stringp is initially NULL, strsep() returns NULL.
 */
char *MPL_strsep(char **stringp, const char *delim)
{
    int i, j;
    char *ret;

    if (!*stringp)
        return NULL;

    ret = *stringp;
    i = 0;
    while (1) {
        if (!ret[i]) {
            *stringp = NULL;
            return ret;
        }
        for (j = 0; delim[j] != '\0'; ++j) {
            if (ret[i] == delim[j]) {
                ret[i] = '\0';
                *stringp = &ret[i+1];
                return ret;
            }
        }
        ++i;
    }
}

