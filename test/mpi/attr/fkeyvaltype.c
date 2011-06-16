/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#include "stdlib.h"

/*
static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a datatype, then make sure that the keyval delete and copy code are still \
executed";
*/

/* Copy increments the attribute value */
int copy_fn( MPI_Datatype oldtype, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, 
	     int *flag);
int copy_fn( MPI_Datatype oldtype, int keyval, void *extra_state,
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
int delete_fn( MPI_Datatype type, int keyval, void *attribute_val, 
	       void *extra_state);
int delete_fn( MPI_Datatype type, int keyval, void *attribute_val, 
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
    MPI_Datatype type, duptype;
    MTestDatatype mstype, mrtype;
    char typename[MPI_MAX_OBJECT_NAME];
    int tnlen;

    MTest_Init( &argc, &argv );

    while (MTestGetDatatypes( &mstype, &mrtype, 1 )) {
	type = mstype.datatype;
	MPI_Type_create_keyval( copy_fn, delete_fn, &keyval, (void *)0 );
	saveKeyval = keyval;   /* in case we need to free explicitly */
	attrval = 1;
	MPI_Type_set_attr( type, keyval, (void*)&attrval );
	/* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
	   is in use in an attribute */
	MPI_Type_free_keyval( &keyval );
	
	/* We create some dummy keyvals here in case the same keyval
	   is reused */
	for (i=0; i<32; i++) {
	    MPI_Type_create_keyval( MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN,
			       &key[i], (void *)0 );
	}

	if (attrval != 1) {
	    errs++;
	    MPI_Type_get_name( type, typename, &tnlen );
	    printf( "attrval is %d, should be 1, before dup in type %s\n",
		     attrval, typename );
	}
	MPI_Type_dup( type, &duptype );
	/* Check that the attribute was copied */
	if (attrval != 2) {
	    errs++;
	    MPI_Type_get_name( type, typename, &tnlen );
	    printf( "Attribute not incremented when type dup'ed (%s)\n",
		     typename );
	}
	MPI_Type_free( &duptype );
	if (attrval != 1) {
	    errs++;
	    MPI_Type_get_name( type, typename, &tnlen );
	    printf( "Attribute not decremented when duptype %s freed\n",
		    typename );
	}
	/* Check that the attribute was freed in the duptype */

	if (!mstype.isBasic) {
	    MPI_Type_get_name( type, typename, &tnlen );
            MTestFreeDatatype(&mstype);
	    /* Check that the original attribute was freed */
	    if (attrval != 0) {
		errs++;
		printf( "Attribute not decremented when type %s freed\n",
			typename );
	    }
	}
	else {
	    /* Explicitly delete the attributes from world and self */
	    MPI_Type_delete_attr( type, saveKeyval );
            if (mstype.buf) {
                free(mstype.buf);
                mstype.buf = 0;
            }
	}
	/* Free those other keyvals */
	for (i=0; i<32; i++) {
	    MPI_Type_free_keyval( &key[i] );
	}
        MTestFreeDatatype(&mrtype);
    }
    MTest_Finalize( errs );
    MPI_Finalize();

    return 0;
  
}
