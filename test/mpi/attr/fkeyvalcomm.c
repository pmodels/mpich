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
static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a communicator, then make sure that the keyval delete and copy code are still \
executed";
*/

/* Function prototypes to keep compilers happy */
int copy_fn( MPI_Comm oldcomm, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, 
	     int *flag);
int delete_fn( MPI_Comm comm, int keyval, void *attribute_val, 
	       void *extra_state);

/* Copy increments the attribute value */
int copy_fn( MPI_Comm oldcomm, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, 
	     int *flag)
{
    /* Copy the address of the attribute */
    *(void **)attribute_val_out = attribute_val_in;
    /* Change the value */
    *(int *)attribute_val_in = *(int *)attribute_val_in + 1;
    /* set flag to 1 to tell comm dup to insert this attribute
       into the new communicator */
    *flag = 1;
    return MPI_SUCCESS;
}

/* Delete decrements the attribute value */
int delete_fn( MPI_Comm comm, int keyval, void *attribute_val, 
	       void *extra_state)
{
    *(int *)attribute_val = *(int *)attribute_val - 1;
    return MPI_SUCCESS;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    int attrval;
    int i, key[32], keyval, saveKeyval;
    MPI_Comm comm, dupcomm;
    MTest_Init( &argc, &argv );

    while (MTestGetIntracomm( &comm, 1 )) {
	if (comm == MPI_COMM_NULL) continue;

	MPI_Comm_create_keyval( copy_fn, delete_fn, &keyval, (void *)0 );
	saveKeyval = keyval;   /* in case we need to free explicitly */
	attrval = 1;
	MPI_Comm_set_attr( comm, keyval, (void*)&attrval );
	/* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
	   is in use in an attribute */
	MPI_Comm_free_keyval( &keyval );
	
	/* We create some dummy keyvals here in case the same keyval
	   is reused */
	for (i=0; i<32; i++) {
	    MPI_Comm_create_keyval( MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN,
			       &key[i], (void *)0 );
	}

	MPI_Comm_dup( comm, &dupcomm );
	/* Check that the attribute was copied */
	if (attrval != 2) {
	    errs++;
	    printf( "Attribute not incremented when comm dup'ed (%s)\n",
		    MTestGetIntracommName() );
	}
	MPI_Comm_free( &dupcomm );
	if (attrval != 1) {
	    errs++;
	    printf( "Attribute not decremented when dupcomm %s freed\n",
		    MTestGetIntracommName() );
	}
	/* Check that the attribute was freed in the dupcomm */

	if (comm != MPI_COMM_WORLD && comm != MPI_COMM_SELF) {
	    MPI_Comm_free( &comm );
	    /* Check that the original attribute was freed */
	    if (attrval != 0) {
		errs++;
		printf( "Attribute not decremented when comm %s freed\n",
			MTestGetIntracommName() );
	    }
	}
	else {
	    /* Explicitly delete the attributes from world and self */
	    MPI_Comm_delete_attr( comm, saveKeyval );
	}
	/* Free those other keyvals */
	for (i=0; i<32; i++) {
	    MPI_Comm_free_keyval( &key[i] );
	}
    }
    MTest_Finalize( errs );
    MPI_Finalize();

    /* The attributes on comm self and world were deleted by finalize 
       (see separate test) */
    
    return 0;
  
}
