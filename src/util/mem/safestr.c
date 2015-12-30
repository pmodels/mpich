/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpichconf.h"
#include "mpimem.h"

/* style: allow:sprintf:1 sig:0 */

/* 
 * This file contains "safe" versions of the various string and printf
 * operations.
 */

/* Append src to dest, but only allow dest to contain n characters (including
   any null, which is always added to the end of the line */
/*@ MPIU_Strnapp - Append to a string with a maximum length

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
int MPIU_Strnapp( char *dest, const char *src, size_t n )
{
    char * restrict d_ptr = dest;
    const char * restrict s_ptr = src;
    register int i;

    /* Get to the end of dest */
    i = (int)n;
    while (i-- > 0 && *d_ptr) d_ptr++;
    if (i <= 0) return 1;

    /* Append.  d_ptr points at first null and i is remaining space. */
    while (*s_ptr && i-- > 0) {
	*d_ptr++ = *s_ptr++;
    }

    /* We allow i >= (not just >) here because the first while decrements
       i by one more than there are characters, leaving room for the null */
    if (i >= 0) { 
	*d_ptr = 0;
	return 0;
    }
    else {
	/* Force the null at the end */
	*--d_ptr = 0;
    
	/* We may want to force an error message here, at least in the
	   debugging version */
	return 1;
    }
}
