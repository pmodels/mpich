/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTestDescrip[] = "Test creating and inserting attributes in \
different orders to ensure that the list management code handles all cases.";
*/

int checkAttrs( MPI_Comm comm, int n, int key[], int attrval[] );
int checkNoAttrs( MPI_Comm comm, int n, int key[] );

int main( int argc, char *argv[] )
{
    int errs = 0;
    int key[3], attrval[3];
    int i;
    MPI_Comm comm;

    MTest_Init( &argc, &argv );

    {
	comm = MPI_COMM_WORLD;
	/* Create key values */
	for (i=0; i<3; i++) {
	    MPI_Keyval_create( MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN,
			       &key[i], (void *)0 );
	    attrval[i] = 1024 * i;
	}
	
	/* Insert attribute in several orders.  Test after put with get,
	 then delete, then confirm delete with get. */

	MPI_Attr_put( comm, key[2], &attrval[2] );
	MPI_Attr_put( comm, key[1], &attrval[1] );
	MPI_Attr_put( comm, key[0], &attrval[0] );

	errs += checkAttrs( comm, 3, key, attrval );
	
	MPI_Attr_delete( comm, key[0] );
	MPI_Attr_delete( comm, key[1] );
	MPI_Attr_delete( comm, key[2] );

	errs += checkNoAttrs( comm, 3, key );
	
	MPI_Attr_put( comm, key[1], &attrval[1] );
	MPI_Attr_put( comm, key[2], &attrval[2] );
	MPI_Attr_put( comm, key[0], &attrval[0] );

	errs += checkAttrs( comm, 3, key, attrval );
	
	MPI_Attr_delete( comm, key[2] );
	MPI_Attr_delete( comm, key[1] );
	MPI_Attr_delete( comm, key[0] );

	errs += checkNoAttrs( comm, 3, key );

	MPI_Attr_put( comm, key[0], &attrval[0] );
	MPI_Attr_put( comm, key[1], &attrval[1] );
	MPI_Attr_put( comm, key[2], &attrval[2] );

	errs += checkAttrs( comm, 3, key, attrval );
	
	MPI_Attr_delete( comm, key[1] );
	MPI_Attr_delete( comm, key[2] );
	MPI_Attr_delete( comm, key[0] );

	errs += checkNoAttrs( comm, 3, key );
	
	for (i=0; i<3; i++) {
	    MPI_Keyval_free( &key[i] );
	}
    }
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}

int checkAttrs( MPI_Comm comm, int n, int key[], int attrval[] )
{
    int errs = 0;
    int i, flag, *val_p;

    for (i=0; i<n; i++) {
	MPI_Attr_get( comm, key[i], &val_p, &flag );
	if (!flag) {
	    errs++;
	    fprintf( stderr, "Attribute for key %d not set\n", i );
	}
	else if (val_p != &attrval[i]) {
	    errs++;
	    fprintf( stderr, "Atribute value for key %d not correct\n",
		     i );
	}
    }

    return errs;
}

int checkNoAttrs( MPI_Comm comm, int n, int key[] )
{
    int errs = 0;
    int i, flag, *val_p;

    for (i=0; i<n; i++) {
	MPI_Attr_get( comm, key[i], &val_p, &flag );
	if (flag) {
	    errs++;
	    fprintf( stderr, "Attribute for key %d set but should be deleted\n", i );
	}
    }

    return errs;
}
	
