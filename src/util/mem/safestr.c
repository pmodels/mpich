/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: safestr.c,v 1.18 2006/10/31 19:47:34 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* style: allow:sprintf:1 sig:0 */

/* 
 * This file contains "safe" versions of the various string and printf
 * operations.  So far, only strncpy is included.
 */

/*
 * MPIU_Strncpy - Copy at most n character.  Stop once a null is reached.
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
/*@ MPIU_Strncpy - Copy a string with a maximum length
  
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
int MPIU_Strncpy( char *dest, const char *src, size_t n )
{
    char * restrict d_ptr = dest;
    const char * restrict s_ptr = src;
    register int i;

    if (n == 0) return 0;

    i = (int)n;
    while (*s_ptr && i-- > 0) {
	*d_ptr++ = *s_ptr++;
    }
    
    if (i > 0) { 
	*d_ptr = 0;
	return 0;
    }
    else {
	/* Force a null at the end of the string (gives better safety 
	   in case the user fails to check the error code) */
	dest[n-1] = 0;
	/* We may want to force an error message here, at least in the
	   debugging version */
	/*printf( "failure in copying %s with length %d\n", src, n ); */
	return 1;
    }
}

/* Append src to dest, but only allow dest to contain n characters (including
   any null, which is always added to the end of the line */
/*@ MPIU_Strnapp - Append to a string with a maximum length

    Input Parameters:
+   instr - String to copy
-   maxlen - Maximum total length of 'outstr'

    Output Parameter:
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

#ifndef HAVE_STRDUP
#ifdef MPIU_Strdup
#undef MPIU_Strdup
#endif
/*@ 
  MPIU_Strdup - Duplicate a string

  Synopsis:
.vb
    char *MPIU_Strdup( const char *str )
.ve

  Input Parameter:
. str - null-terminated string to duplicate

  Return value:
  A pointer to a copy of the string, including the terminating null.  A
  null pointer is returned on error, such as out-of-memory.

  Notes:
  Like 'MPIU_Malloc' and 'MPIU_Free', this will often be implemented as a 
  macro but may use 'MPIU_trstrdup' to provide a tracing version.

  Module:
  Utility
  @*/
char *MPIU_Strdup( const char *str )
{
    char * restrict p = (char *)MPIU_Malloc( strlen(str) + 1 );
    const char * restrict in_p = str;
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

/* 
 * We need an snprintf replacement for systems without one
 */
#ifndef HAVE_SNPRINTF
/* 
 * This is an approximate form which is suitable for most uses within
 * the MPICH code
 */
int MPIU_Snprintf( char *str, size_t size, const char *format, ... )
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
		   character */
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
		val = va_arg( list, int );
		sprintf( tmp, "%d", val );
		if (width > 0) {
		    int tmplen = strlen(tmp);
		    /* If a width was specified, pad with spaces on the
		       left (on the right if %-3d given; not implemented yet */
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
		val = va_arg( list, int );
		sprintf( tmp, "%x", val );
		if (width > 0) {
		    int tmplen = strlen(tmp);
		    /* If a width was specified, pad with spaces on the
		       left (on the right if %-3d given; not implemented yet */
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
		int val;
		char tmp[20];
		char *t = tmp;
		/* Get the argument, of integer type */
		val = va_arg( list, int );
		sprintf( tmp, "%p", val );
		if (width > 0) {
		    int tmplen = strlen(tmp);
		    /* If a width was specified, pad with spaces on the
		       left (on the right if %-3d given; not implemented yet */
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
		s_arg = va_arg( list, char * );
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

    if (size-- > 0) *out_str++ = '\0';

    n = (int)(out_str - str);
    return n;
}
#endif
