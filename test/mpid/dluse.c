/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 */
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main( int argc, char *argv[] )
{
    void *handle;
    int (*init)(void);
    int (*finalize)(int);
    int *counter;
    int errs = 0, rc;

    /* We allow different extensions for the shared libraries here, 
     as OSX uses .dylib and Cygwin may use .dll . */
    handle = dlopen( "./libconftest."## #SHLIBEXT, RTLD_LAZY );
    if (!handle) {
	fprintf( stderr, "Could not open test library: %s\n", dlerror() );
	exit(1);
    }

    init = (int (*)(void))dlsym( handle, "init" );
    counter = (int *)dlsym( handle, "counter" );
    finalize = (int (*)(int))dlsym( handle, "finalize" );
    if (!init || !counter || !finalize) {
	errs++;
	fprintf( stderr, "Could not load a function or variable\n" );
	exit(1);
    }

    if (*counter != 1) {
	errs++;
	fprintf( stderr, "counter value is %d, expected 1\n" );
    }
    (*init)();
    if (*counter != 2) {
	errs++;
	fprintf( stderr, "counter value is %d, expected 2\n" );
    }
    rc = (*finalize)(2);
    if (rc != 1) {
	errs++;
	fprintf( stderr, "finalize returned failure\n" );
    }
    dlclose( handle );

    printf( "Found %d errors\n", errs );
    return 0;
}

int upcall( int a )
{
    return a + 1;
}
