/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: uni_pmiutil.c,v 1.2 2003/02/17 14:23:02 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Utility functions associated with PMI implementation, but not part of
   the PMI interface itself.  Reading and writing on pipes, signals, and parsing
   key=value messages
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "uni_pmiutil.h"

#define MAXVALLEN 1024
#define MAXKEYLEN   32

/* These are not the keyvals in the keyval space that is part of the PMI specification.
   They are just part of this implementation's internal utilities.
*/
#ifdef FOO
struct PMIU_keyval_pairs {
    char key[MAXKEYLEN];
    char value[MAXVALLEN];	
};
struct PMIU_keyval_pairs PMIU_keyval_tab[64];
int  PMIU_keyval_tab_idx;

void PMIU_printf( int print_flag, char *fmt, ... )
{
    va_list ap;

    if ( print_flag ) {
	f print f( stderr, "[%s]: ", PMIU_print_id );
	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
	fflush( stderr );
    }
}

int PMIU_parse_keyvals( char *st )
{
    char *p, *keystart, *valstart;

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
        MPIU_Strncpy( PMIU_keyval_tab[PMIU_keyval_tab_idx].key, keystart, MAXKEYLEN );
	PMIU_keyval_tab[PMIU_keyval_tab_idx].key[p - keystart] = '\0'; /* store key */

	valstart = ++p;			/* start of value */
	while ( *p != ' ' && *p != '\n' && *p != '\0' )
	    p++;
        MPIU_Strncpy( PMIU_keyval_tab[PMIU_keyval_tab_idx].value, valstart, MAXVALLEN );
	PMIU_keyval_tab[PMIU_keyval_tab_idx].value[p - valstart] = '\0'; /* store value */
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

char *PMIU_getval( const char *keystr, char *valstr )
{
    int i;

    for (i=0; i < PMIU_keyval_tab_idx; i++) {
       if ( strncmp( keystr, PMIU_keyval_tab[i].key, MAXKEYLEN ) == 0 ) { 
	    MPIU_Strncpy( valstr, PMIU_keyval_tab[i].value, MAXVALLEN );
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
       if ( strncmp( keystr, PMIU_keyval_tab[i].key, MAXKEYLEN ) == 0 )
	   MPIU_Strncpy( PMIU_keyval_tab[i].value, valstr, MAXVALLEN );
    }
}
#endif
