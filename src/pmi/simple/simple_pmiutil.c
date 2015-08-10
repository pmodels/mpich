/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Allow fprintf to logfile */
/* style: allow:fprintf:1 sig:0 */

/* Utility functions associated with PMI implementation, but not part of
   the PMI interface itself.  Reading and writing on pipes, signals, and parsing
   key=value messages
*/

#include "mpichconf.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <stdarg.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include "mpl.h"

#include "simple_pmiutil.h"

/* Use the memory definitions from mpich/src/include */
#include "mpimem.h"

#define MAXVALLEN 1024
#define MAXKEYLEN   32

/* These are not the keyvals in the keyval space that is part of the 
   PMI specification.
   They are just part of this implementation's internal utilities.
*/
struct PMIU_keyval_pairs {
    char key[MAXKEYLEN];
    char value[MAXVALLEN];	
};
static struct PMIU_keyval_pairs PMIU_keyval_tab[64] = { { {0}, {0} } };
static int  PMIU_keyval_tab_idx = 0;

/* This is used to prepend printed output.  Set the initial value to 
   "unset" */
static char PMIU_print_id[PMIU_IDSIZE] = "unset";

void PMIU_Set_rank( int PMI_rank )
{
    MPL_snprintf( PMIU_print_id, PMIU_IDSIZE, "cli_%d", PMI_rank );
}
void PMIU_SetServer( void )
{
    MPIU_Strncpy( PMIU_print_id, "server", PMIU_IDSIZE );
}

/* Note that vfprintf is part of C89 */

/* style: allow:fprintf:1 sig:0 */
/* style: allow:vfprintf:1 sig:0 */
/* This should be combined with the message routines */
void PMIU_printf( int print_flag, const char *fmt, ... )
{
    va_list ap;
    static FILE *logfile= 0;
    
    /* In some cases when we are debugging, the handling of stdout or
       stderr may be unreliable.  In that case, we make it possible to
       select an output file. */
    if (!logfile) {
	char *p;
	p = getenv("PMI_USE_LOGFILE");
	if (p) {
	    char filename[1024];
	    p = getenv("PMI_ID");
	    if (p) {
		MPL_snprintf( filename, sizeof(filename), 
			       "testclient-%s.out", p );
		logfile = fopen( filename, "w" );
	    }
	    else {
		logfile = fopen( "testserver.out", "w" );
	    }
	}
	else 
	    logfile = stderr;
    }

    if ( print_flag ) {
	/* MPL_error_printf( "[%s]: ", PMIU_print_id ); */
	/* FIXME: Decide what role PMIU_printf should have (if any) and
	   select the appropriate MPIU routine */
	fprintf( logfile, "[%s]: ", PMIU_print_id );
	va_start( ap, fmt );
	vfprintf( logfile, fmt, ap );
	va_end( ap );
	fflush( logfile );
    }
}

#define MAX_READLINE 1024
/* 
 * Return the next newline-terminated string of maximum length maxlen.
 * This is a buffered version, and reads from fd as necessary.  A
 */
int PMIU_readline( int fd, char *buf, int maxlen )
{
    static char readbuf[MAX_READLINE];
    static char *nextChar = 0, *lastChar = 0;  /* lastChar is really one past 
						  last char */
    static int lastfd = -1;
    ssize_t n;
    int     curlen;
    char    *p, ch;

    /* Note: On the client side, only one thread at a time should 
       be calling this, and there should only be a single fd.  
       Server side code should not use this routine (see the 
       replacement version in src/pm/util/pmiserv.c) */
    if (nextChar != lastChar && fd != lastfd) {
	MPL_internal_error_printf( "Panic - buffer inconsistent\n" );
	return -1;
    }

    p      = buf;
    curlen = 1;    /* Make room for the null */
    while (curlen < maxlen) {
	if (nextChar == lastChar) {
	    lastfd = fd;
	    do {
		n = read( fd, readbuf, sizeof(readbuf)-1 );
	    } while (n == -1 && errno == EINTR);
	    if (n == 0) {
		/* EOF */
		break;
	    }
	    else if (n < 0) {
		/* Error.  Return a negative value if there is no
		   data.  Save the errno in case we need to return it
		   later. */
		if (curlen == 1) {
		    curlen = 0;
		}
		break;
	    }
	    nextChar = readbuf;
	    lastChar = readbuf + n;
	    /* Add a null at the end just to make it easier to print
	       the read buffer */
	    readbuf[n] = 0;
	    /* FIXME: Make this an optional output */
	    /* printf( "Readline %s\n", readbuf ); */
	}
	
	ch   = *nextChar++;
	*p++ = ch;
	curlen++;
	if (ch == '\n') break;
    }

    /* We null terminate the string for convenience in printing */
    *p = 0;

    /* Return the number of characters, not counting the null */
    return curlen-1;
}

int PMIU_writeline( int fd, char *buf )	
{
    ssize_t size, n;

    size = strlen( buf );
    if ( size > PMIU_MAXLINE ) {
	buf[PMIU_MAXLINE-1] = '\0';
	PMIU_printf( 1, "write_line: message string too big: :%s:\n", buf );
    }
    else if ( buf[strlen( buf ) - 1] != '\n' )  /* error:  no newline at end */
	    PMIU_printf( 1, "write_line: message string doesn't end in newline: :%s:\n",
		       buf );
    else {
	do {
	    n = write( fd, buf, size );
	} while (n == -1 && errno == EINTR);

	if ( n < 0 ) {
	    PMIU_printf( 1, "write_line error; fd=%d buf=:%s:\n", fd, buf );
	    perror("system msg for write_line failure ");
	    return(-1);
	}
	if ( n < size)
	    PMIU_printf( 1, "write_line failed to write entire message\n" );
    }
    return 0;
}

/*
 * Given an input string st, parse it into internal storage that can be
 * queried by routines such as PMIU_getval.
 */
int PMIU_parse_keyvals( char *st )
{
    char *p, *keystart, *valstart;
    int  offset;

    if ( !st )
	return( -1 );

    PMIU_keyval_tab_idx = 0;
    p = st;
    while ( 1 ) {
	while ( *p == ' ' )
	    p++;
	/* got non-blank */
	if ( *p == '=' ) {
	    PMIU_printf( 1, "PMIU_parse_keyvals:  unexpected = at character %d in %s\n",
		       p - st, st );
	    return( -1 );
	}
	if ( *p == '\n' || *p == '\0' )
	    return( 0 );	/* normal exit */
	/* got normal character */
	keystart = p;		/* remember where key started */
	while ( *p != ' ' && *p != '=' && *p != '\n' && *p != '\0' )
	    p++;
	if ( *p == ' ' || *p == '\n' || *p == '\0' ) {
	    PMIU_printf( 1,
       "PMIU_parse_keyvals: unexpected key delimiter at character %d in %s\n",
		       p - st, st );
	    return( -1 );
	}
	/* Null terminate the key */
	*p = 0;
	/* store key */
        MPIU_Strncpy( PMIU_keyval_tab[PMIU_keyval_tab_idx].key, keystart, 
		      MAXKEYLEN );

	valstart = ++p;			/* start of value */
	while ( *p != ' ' && *p != '\n' && *p != '\0' )
	    p++;
	/* store value */
        MPIU_Strncpy( PMIU_keyval_tab[PMIU_keyval_tab_idx].value, valstart, 
		      MAXVALLEN );
	offset = (int)(p - valstart);
	/* When compiled with -fPIC, the pgcc compiler generates incorrect
	   code if "p - valstart" is used instead of using the 
	   intermediate offset */
	PMIU_keyval_tab[PMIU_keyval_tab_idx].value[offset] = '\0';  
	PMIU_keyval_tab_idx++;
	if ( *p == ' ' )
	    continue;
	if ( *p == '\n' || *p == '\0' )
	    return( 0 );	/* value has been set to empty */
    }
}

void PMIU_dump_keyvals( void )
{
    int i;
    for (i=0; i < PMIU_keyval_tab_idx; i++) 
	PMIU_printf(1, "  %s=%s\n",PMIU_keyval_tab[i].key, PMIU_keyval_tab[i].value);
}

char *PMIU_getval( const char *keystr, char *valstr, int vallen )
{
    int i, rc;
    
    for (i = 0; i < PMIU_keyval_tab_idx; i++) {
	if ( strcmp( keystr, PMIU_keyval_tab[i].key ) == 0 ) { 
	    rc = MPIU_Strncpy( valstr, PMIU_keyval_tab[i].value, vallen );
	    if (rc != 0) {
		PMIU_printf( 1, "MPIU_Strncpy failed in PMIU_getval\n" );
		return NULL;
	    }
	    return valstr;
       } 
    }
    valstr[0] = '\0';
    return NULL;
}

void PMIU_chgval( const char *keystr, char *valstr )
{
    int i;
    
    for ( i = 0; i < PMIU_keyval_tab_idx; i++ ) {
	if ( strcmp( keystr, PMIU_keyval_tab[i].key ) == 0 ) {
	    MPIU_Strncpy( PMIU_keyval_tab[i].value, valstr, MAXVALLEN - 1 );
	    PMIU_keyval_tab[i].value[MAXVALLEN - 1] = '\0';
	}
    }
}
