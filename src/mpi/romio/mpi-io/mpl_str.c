/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "romioconf.h"
#ifndef ROMIO_INSIDE_MPICH

#include <stdio.h>
#include <stdlib.h>     /* posix_memalign() */

#include <stdarg.h>     /* va_start() va_end() */
#include <string.h>     /* strchr() */
#include <ctype.h>      /* isdigit() */
#include <assert.h>

#include <sys/types.h>  /* getpid() */
#include <unistd.h>     /* getpid() */
#include <time.h>       /* time() */

#include <adio.h>       /* PATH_MAX */

#ifndef HAVE_MPL_CREATE_PATHNAME

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
        snprintf(dest_filename, PATH_MAX, "%s/%s.%u.%u%c", dirname, prefix,
                     rdm, pid, is_dir ? '/' : '\0');
    } else {
        snprintf(dest_filename, PATH_MAX, "%s.%u.%u%c", prefix, rdm, pid, is_dir ? '/' : '\0');
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

#ifndef HAVE_MPL_ALIGNED_ALLOC
void *MPL_aligned_alloc(size_t alignment, size_t size, MPL_memory_class class)
{
    void *ptr;
    int ret;

    ret = posix_memalign(&ptr, alignment, size);
    if (ret != 0)
        return NULL;
    return ptr;
}
#endif
#endif
