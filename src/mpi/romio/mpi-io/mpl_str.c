/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

#include <stdarg.h>     /* va_start() va_end() */
#include <string.h>     /* strchr() */
#include <ctype.h>      /* isdigit() */
#include <assert.h>

#ifndef HAVE_MPL_SNPRINTF
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
        } else {
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
            } else {
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
                            size_t tmplen = strlen(tmp);
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
                            size_t tmplen = strlen(tmp);
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
                            size_t tmplen = strlen(tmp);
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

#ifndef HAVE_MPL_CREATE_PATHNAME
#include <sys/types.h>  /* getpid() */
#include <unistd.h>     /* getpid() */
#include <time.h>       /* time() */

#include <adio.h>       /* PATH_MAX */

static unsigned int xorshift_rand(void)
{
    /* time returns long; keep the lower and most significant 32 bits */
    unsigned int val = time(NULL) & 0xffffffff;

    /* Marsaglia's xorshift random number generator */
    val ^= val << 13;
    val ^= val >> 17;
    val ^= val << 5;

    return val;
}

/*@ MPL_create_pathname - Generate a random pathname

Input Parameters:
+   dirname - String containing the path of the parent dir (current dir if NULL)
+   prefix - String containing the prefix of the generated name
-   is_dir - Boolean to tell if the path should be treated as a directory

Output Parameters:
.   dest_filename - String to copy the generated path name

    Notes:
    dest_filename should point to a preallocated buffer of PATH_MAX size.

  Module:
  Utility
  @*/
void MPL_create_pathname(char *dest_filename, const char *dirname,
                         const char *prefix, const int is_dir)
{
    /* Generate a random number which doesn't interfere with user application */
    const unsigned int rdm = xorshift_rand();
    const unsigned int pid = (unsigned int) getpid();

    if (dirname) {
        MPL_snprintf(dest_filename, PATH_MAX, "%s/%s.%u.%u%c", dirname, prefix,
                     rdm, pid, is_dir ? '/' : '\0');
    } else {
        MPL_snprintf(dest_filename, PATH_MAX, "%s.%u.%u%c", prefix, rdm, pid, is_dir ? '/' : '\0');
    }
}
#endif

#ifndef HAVE_MPL_STRNAPP
/*@ MPL_strnapp - Append to a string with a maximum length

Input Parameters:
+   instr - String to copy
-   maxlen - Maximum total length of 'outstr'

Output Parameters:
.   outstr - String to copy into

    Notes:
    This routine is similar to 'strncat' except that the 'maxlen' argument
    is the maximum total length of 'outstr', rather than the maximum
    number of characters to move from 'instr'.  Thus, this routine is
    easier to use when the declared size of 'instr' is known.

  Module:
  Utility
  @*/
int MPL_strnapp(char *dest, const char *src, size_t n)
{
    char *restrict d_ptr = dest;
    const char *restrict s_ptr = src;
    register int i;

    /* Get to the end of dest */
    i = (int) n;
    while (i-- > 0 && *d_ptr)
        d_ptr++;
    if (i <= 0)
        return 1;

    /* Append.  d_ptr points at first null and i is remaining space. */
    while (*s_ptr && i-- > 0) {
        *d_ptr++ = *s_ptr++;
    }

    /* We allow i >= (not just >) here because the first while decrements
     * i by one more than there are characters, leaving room for the null */
    if (i >= 0) {
        *d_ptr = 0;
        return 0;
    } else {
        /* Force the null at the end */
        *--d_ptr = 0;

        /* We may want to force an error message here, at least in the
         * debugging version */
        return 1;
    }
}
#endif
