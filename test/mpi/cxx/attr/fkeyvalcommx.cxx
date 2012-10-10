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
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a communicator, then make sure that the keyval delete and copy code are still \
executed";

/* Copy increments the attribute value */
int copy_fn( const MPI::Comm &oldcomm, int keyval, void *extra_state,
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
int delete_fn( MPI::Comm &comm, int keyval, void *attribute_val, 
	       void *extra_state)
{
    *(int *)attribute_val = *(int *)attribute_val - 1;
    return MPI::SUCCESS;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    int attrval;
    int i, key[32], keyval, saveKeyval;
    MPI::Intracomm comm, dupcomm;

    MTest_Init();

    while (MTestGetIntracomm( comm, 1 )) {
	if (comm == MPI::COMM_NULL) continue;

	keyval = MPI::Comm::Create_keyval( copy_fn, delete_fn, (void *)0 );
	saveKeyval = keyval;   /* in case we need to free explicitly */
	attrval = 1;
	comm.Set_attr( keyval, &attrval );
	/* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
	   is in use in an attribute */
	MPI::Comm::Free_keyval( keyval );
	
	/* We create some dummy keyvals here in case the same keyval
	   is reused */
	for (i=0; i<32; i++) {
	    key[i] = MPI::Comm::Create_keyval( MPI::Comm::NULL_COPY_FN, 
					       MPI::Comm::NULL_DELETE_FN,
					       (void *)0 );
	}

	dupcomm = comm.Dup();
	/* Check that the attribute was copied */
	if (attrval != 2) {
	    errs++;
	    cout << "Attribute not incremented when comm dup'ed, attrval = " <<
		attrval << " should be 2 in communicator " << 
		MTestGetIntracommName() << "\n";
	}
	dupcomm.Free();
	if (attrval != 1) {
	    errs++;
	    cout << "Attribute not decremented when comm dup'ed, attrval = " <<
		attrval << " should be 1 in communicator " << 
		MTestGetIntracommName() << "\n";
	}
	/* Check that the attribute was freed in the dupcomm */

	if (comm != MPI::COMM_WORLD && comm != MPI::COMM_SELF) {
	    comm.Free();
	    /* Check that the original attribute was freed */
	    if (attrval != 0) {
		errs++;
		printf( "Attribute not decremented when comm %s freed\n",
			MTestGetIntracommName() );
	    }
	}
	else {
	    /* Explicitly delete the attributes from world and self */
	    comm.Delete_attr( saveKeyval );
	}
	/* Free those other keyvals */
	for (i=0; i<32; i++) {
	    MPI::Comm::Free_keyval( key[i] );
	}
    }
    MTest_Finalize( errs );
    MPI::Finalize();

    /* The attributes on comm self and world were deleted by finalize 
       (see separate test) */
    
    return 0;
  
}
