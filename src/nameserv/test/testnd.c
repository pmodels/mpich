/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* 
 * A simple test program for name service routines.  This does not 
 * require the MPI library
 */

#include <stdio.h>
#include <stdarg.h>
/* This is incomplete for the purposes of testing */
typedef struct { int handle; } MPID_Info;
#define MPID_INFO_NULL ((MPID_Info *)0)
#include "namepub.h"

void Error( const char *fmat, ... );
int MPIR_Err_create_code( int old, int kind, const char *fname, 
			  int line, int class, ... );

int main( int argc, char *argv[] )
{
    MPID_NS_Handle ns;
    char           port[1024];
    int            rc;

    /* Create a name service */
    rc = MPID_NS_Create( MPID_INFO_NULL, &ns );
    if (rc) {
	Error( "Could not create name service; rc = %d\n", rc );
    }
    /* publish several names */
    rc = MPID_NS_Publish( ns, MPID_INFO_NULL, "name1", "foo$$12" );
    if (rc) {
	Error( "Could not publish name1; rc = %d\n", rc );
    }
    rc = MPID_NS_Publish( ns, MPID_INFO_NULL, "namea", "bar--14" );
    if (rc) {
	Error( "Could not publish namea; rc = %d\n", rc );
    }
    rc = MPID_NS_Publish( ns, MPID_INFO_NULL, "1-2-3", "testname" );
    if (rc) {
	Error( "Could not publish 1-2-3; rc = %d\n", rc );
    }

    /* Try look ups */
    rc = MPID_NS_Lookup( ns, MPID_INFO_NULL, "name1", port );
    if (rc) {
	Error( "Could not lookup name1; rc = %d\n", rc );
    }
    else {
	if (strcmp( port, "foo$$12" ) != 0) {
	    Error( "Wrong value for port, got %s\n", port );
	}
    }

    rc = MPID_NS_Lookup( ns, MPID_INFO_NULL, "namea", port );
    if (rc) {
	Error( "Could not lookup namea; rc = %d\n", rc );
    }
    else {
	if (strcmp( port, "bar--14" ) != 0) {
	    Error( "Wrong value for port, got %s\n", port );
	}
    }

    rc = MPID_NS_Lookup( ns, MPID_INFO_NULL, "1-2-3", port );
    if (rc) {
	Error( "Could not lookup 1-2-3; rc = %d\n", rc );
    }
    else {
	if (strcmp( port, "testname" ) != 0) {
	    Error( "Wrong value for port, got %s\n", port );
	}
    }

    /* Try a name that isn't published */
    port[0] = 0;
    rc = MPID_NS_Lookup( ns, MPID_INFO_NULL, "name", port );
    if (!rc) {
	Error( "Found port (%s) for unpublished name\n", port );
    }

    rc = MPID_NS_Publish( ns, MPID_INFO_NULL, "name 1", "foo 12" );
    if (rc) {
	Error( "Could not publish \"name 1\"; rc = %d\n", rc );
    }
    rc = MPID_NS_Lookup( ns, MPID_INFO_NULL, "name 1", port );
    if (rc) {
	Error( "Could not lookup \"name 1\"; rc = %d\n", rc );
    }
    else {
	if (strcmp( port, "foo 12" ) != 0) {
	    Error( "Wrong value for port, got %s\n", port );
	}
    }

    /* Note that there are some restrictions in the file-based version */
    rc = MPID_NS_Publish( ns, MPID_INFO_NULL, "name/1", "foo/12a" );
    if (rc) {
	/* Allow publish to fail with some names */
	;
    }
    else {
	rc = MPID_NS_Lookup( ns, MPID_INFO_NULL, "name/1", port );
	if (rc) {
	    Error( "Could not lookup name/1; rc = %d\n", rc );
	}
	else {
	    if (strcmp( port, "foo/12a" ) != 0) {
		Error( "Wrong value for port, got %s\n", port );
	    }
	}
	rc = MPID_NS_Unpublish( ns, MPID_INFO_NULL, "name/1" );
	if (rc) {
	    Error( "Could not unpublish name/1; rc = %d\n", rc );
	}
    }

    /* Try to unpublish the names */
    rc = MPID_NS_Unpublish( ns, MPID_INFO_NULL, "name1" );
    if (rc) {
	Error( "Could not unpublish name1; rc = %d\n", rc );
    }
    rc = MPID_NS_Unpublish( ns, MPID_INFO_NULL, "name 1" );
    if (rc) {
	Error( "Could not unpublish \"name 1\"; rc = %d\n", rc );
    }
    rc = MPID_NS_Unpublish( ns, MPID_INFO_NULL, "namea" );
    if (rc) {
	Error( "Could not unpublish namea; rc = %d\n", rc );
    }
    rc = MPID_NS_Unpublish( ns, MPID_INFO_NULL, "1-2-3" );
    if (rc) {
	Error( "Could not unpublish 1-2-3; rc = %d\n", rc );
    }

    /* If we make it to the end, there are no errors */
    printf( " No Errors\n" );

    return 0;
}

void Error( const char *fmat, ... )
{
    va_list list;
    va_start( list, fmat );
    vprintf( fmat, list );
    va_end(list);
    fflush( stdout );
    exit( 1 );
}

#ifdef STANDALONE
int MPIR_Err_create_code( int old, int kind, const char *fname, 
			  int line, int class, ... )
{
    return class;
}
#endif
