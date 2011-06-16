/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTestDescrip[] = "Test creating and inserting attributes in \
different orders to ensure that the list management code handles all cases.";
*/

int checkAttrs( MPI_Comm, int, int [], int [] );
int delete_fn( MPI_Comm, int, void *, void *);

#define NKEYS 5
static int key[NKEYS];      /* Keys in creation order */
static int keyorder[NKEYS]; /* Index (into key) of keys in order added to comm 
			    (key[keyorder[0]] is first set) */
static int nkeys = 0;
static int ncall = 0;
static int errs  = 0;
/* 
 * Test that attributes on comm self are deleted in LIFO order 
 */

int main( int argc, char *argv[] )
{
    int      attrval[10];
    int      wrank, i;
    MPI_Comm comm;

    MPI_Init( &argc, &argv );

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );

    comm = MPI_COMM_SELF;
    
    /* Create key values */
    for (nkeys=0; nkeys<NKEYS; nkeys++) {
	MPI_Comm_create_keyval( MPI_NULL_COPY_FN, delete_fn,
				&key[nkeys], (void *)0 );
	attrval[nkeys] = 1024 * nkeys;
    }
    
    /* Insert attribute in several orders.  Test after put with get,
       then delete, then confirm delete with get. */
    
    MPI_Comm_set_attr( comm, key[3], &attrval[3] ); keyorder[0] = 3;
    MPI_Comm_set_attr( comm, key[2], &attrval[2] ); keyorder[1] = 2;
    MPI_Comm_set_attr( comm, key[0], &attrval[0] ); keyorder[2] = 0;
    MPI_Comm_set_attr( comm, key[1], &attrval[1] ); keyorder[3] = 1;
    MPI_Comm_set_attr( comm, key[4], &attrval[4] ); keyorder[4] = 4;
    
    errs += checkAttrs( comm, NKEYS, key, attrval );
    
    for (i=0; i<NKEYS; i++) {
	/* Save the key value so that we can compare it in the 
	   delete function */
	int keyval = key[i];
	MPI_Comm_free_keyval( &keyval );
    }
	
    MPI_Finalize();
    
    if (wrank == 0) {
	if (ncall != nkeys) {
	    printf( "Deleted %d keys but should have deleted %d\n", 
		    ncall, nkeys );
	    errs++;
	}
	if (errs == 0) printf( " No Errors\n" );
	else printf( " Found %d errors\n", errs );
    }
    return 0;
  
}

int checkAttrs( MPI_Comm comm, int n, int lkey[], int attrval[] )
{
    int lerrs = 0;
    int i, flag, *val_p;

    for (i=0; i<n; i++) {
	MPI_Comm_get_attr( comm, lkey[i], &val_p, &flag );
	if (!flag) {
	    lerrs++;
	    fprintf( stderr, "Attribute for key %d not set\n", i );
	}
	else if (val_p != &attrval[i]) {
	    lerrs++;
	    fprintf( stderr, "Atribute value for key %d not correct\n",
		     i );
	}
    }

    return lerrs;
}

/* We *should* be deleting key[keyorder[nkeys-ncall]] */
int delete_fn( MPI_Comm comm, int keyval, void *attribute_val, 
	       void *extra_state)
{
    if (ncall >= nkeys) {
	printf( "delete function called too many times!\n" );
	errs++;
    }

    /* As of MPI 2.2, the order of deletion of attributes on 
       MPI_COMM_SELF is defined */
    if (MPI_VERSION > 2 || (MPI_VERSION == 2 && MPI_SUBVERSION >= 2)) {
	if (keyval != key[keyorder[nkeys-1-ncall]]) {
	    printf( "Expected key # %d but found key with value %d\n", 
		    keyorder[nkeys-1-ncall], keyval );
	    errs++;
	}
    }
    ncall++;
    return MPI_SUCCESS;
}

/*
int checkNoAttrs( MPI_Comm comm, int n, int lkey[] )
{
    int lerrs = 0;
    int i, flag, *val_p;

    for (i=0; i<n; i++) {
	MPI_Comm_get_attr( comm, lkey[i], &val_p, &flag );
	if (flag) {
	    lerrs++;
	    fprintf( stderr, "Attribute for key %d set but should be deleted\n", i );
	}
    }

    return lerrs;
}
*/
