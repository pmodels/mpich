/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"


#include "mpitestconf.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include <stdio.h>
#include "mpitestcxx.h"
#include <stdlib.h>

static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a datatype, then make sure that the keyval delete and copy code are still \
executed";

/* Copy increments the attribute value */
int copy_fn( const MPI::Datatype &oldtype, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, 
	     bool &flag)
{
    /* Copy the address of the attribute */
    *(void **)attribute_val_out = attribute_val_in;
    /* Change the value */
    *(int *)attribute_val_in = *(int *)attribute_val_in + 1;
    /* set flag to 1 to tell comm dup to insert this attribute
       into the new communicator */
    flag = 1;
    return MPI::SUCCESS;
}

/* Delete decrements the attribute value */
int delete_fn( MPI::Datatype &type, int keyval, void *attribute_val, 
	       void *extra_state)
{
    *(int *)attribute_val = *(int *)attribute_val - 1;
    return MPI::SUCCESS;
}

int main( int argc, char *argv[] )
{
    int           errs = 0;
    int           attrval;
    int           i, key[32], keyval, saveKeyval;
    MPI::Datatype type, duptype;
    MTestDatatype mstype, mrtype;
    char dtypename[MPI_MAX_OBJECT_NAME];
    int tnlen;

    MTest_Init();

    while (MTestGetDatatypes( &mstype, &mrtype, 1 )) {
	type = mstype.datatype;
	keyval = MPI::Datatype::Create_keyval( copy_fn, delete_fn, (void *)0 );
	saveKeyval = keyval;   /* in case we need to free explicitly */
	attrval = 1;
	type.Set_attr( keyval, &attrval );
	/* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
	   is in use in an attribute */
	MPI::Datatype::Free_keyval( keyval );
	
	/* We create some dummy keyvals here in case the same keyval
	   is reused */
	for (i=0; i<32; i++) {
	    key[i] = MPI::Datatype::Create_keyval( 
		MPI::Datatype::NULL_COPY_FN, MPI::Datatype::NULL_DELETE_FN,
		(void *)0 );
	}

	if (attrval != 1) {
	    errs++;
	    type.Get_name( dtypename, tnlen );
	    cout << "attrval is " << attrval << ", should be 1, before dup in type " << dtypename << "\n";
	}
	duptype = type.Dup();
	/* Check that the attribute was copied */
	if (attrval != 2) {
	    errs++;
	    type.Get_name( dtypename, tnlen );
	    cout << "Attribute not incremented when type dup'ed ( " <<
		dtypename << ")\n";
	}
	duptype.Free();
	if (attrval != 1) {
	    errs++;
	    type.Get_name( dtypename, tnlen );
	    cout << "Attribute not decremented when duptype " << dtypename <<
		" freed\n";
	}
	/* Check that the attribute was freed in the duptype */

	if (!mstype.isBasic) {
	    type.Get_name( dtypename, tnlen );
            MTestFreeDatatype(&mstype);
	    /* Check that the original attribute was freed */
	    if (attrval != 0) {
		errs++;
		cout << "Attribute not decremented when type " << dtypename <<
		    " freed\n";
	    }
	}
	else {
	    /* Explicitly delete the attributes from world and self */
	    type.Delete_attr( saveKeyval );
            if (mstype.buf) {
                free(mstype.buf);
                mstype.buf = 0;
            }
	}
	/* Free those other keyvals */
	for (i=0; i<32; i++) {
	    MPI::Datatype::Free_keyval( key[i] );
	}
        MTestFreeDatatype(&mrtype);
    }
    MTest_Finalize( errs );
    MPI::Finalize();

    return 0;
  
}
