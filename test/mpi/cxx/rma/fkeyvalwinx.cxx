/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

static char MTestDescrip[] = "Test freeing keyvals while still attached to \
a win, then make sure that the keyval delete code are still \
executed";

/* Copy increments the attribute value */
/* Note that we can really ignore this because there is no win dup */
int copy_fn( const MPI::Win &oldwin, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, 
	     bool &flag )
{
    /* Copy the address of the attribute */
    *(void **)attribute_val_out = attribute_val_in;
    /* Change the value */
    *(int *)attribute_val_in = *(int *)attribute_val_in + 1;
    flag = 1;
    return MPI::SUCCESS;
}

/* Delete decrements the attribute value */
int delete_fn( MPI::Win &win, int keyval, void *attribute_val, 
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
    MPI::Win win;
    MTest_Init();

    while (MTestGetWin( win, 0 )) {
	if (win == MPI::WIN_NULL) continue;

	keyval = MPI::Win::Create_keyval( copy_fn, delete_fn, (void *)0 );
	saveKeyval = keyval;   /* in case we need to free explicitly */
	attrval = 1;
	win.Set_attr( keyval, &attrval );
	/* See MPI-1, 5.7.1.  Freeing the keyval does not remove it if it
	   is in use in an attribute */
	MPI::Win::Free_keyval( keyval );
	
	/* We create some dummy keyvals here in case the same keyval
	   is reused */
	for (i=0; i<32; i++) {
	    key[i] = MPI::Win::Create_keyval( 
		MPI::Win::NULL_COPY_FN, MPI::Win::NULL_DELETE_FN, (void *)0 );
	}

	MTestFreeWin( win );
	/* Check that the original attribute was freed */
	if (attrval != 0) {
	    errs++;
	    cout << "Attribute not decremented when win " << 
		MTestGetWinName() << " freed\n";
	}
	/* Free those other keyvals */
	for (i=0; i<32; i++) {
	    MPI::Win::Free_keyval( key[i] );
	}
    }
    MTest_Finalize( errs );
    MPI::Finalize();

    return 0;
}
