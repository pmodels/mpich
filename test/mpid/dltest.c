/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This provides a simple test of dynamically loading a library.  If 
   NO_UPCALL is defined, this library only provides routines; otherwise,
   it will make use of the routine upcall provided in the program that is 
   loading this library */
#ifndef NO_UPCALL
extern int upcall( int );
#endif

int counter = 1;

int init(void) {
    int a = counter;
    counter++;
#ifndef NO_UPCALL
    a = upcall( a );
    if (a != counter) {
	counter --;
    }
#endif
}
int finalize( int offset )
{
    int rc = 1;
    if (counter != offset) {
	rc = 0;
    }
    return rc;
}
