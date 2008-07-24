/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int i, count, err, errs = 0;
    MPI_Datatype type_ia;
    char omessage[256], imessage[256];
    MPI_Status recvstatus, sendstatus;
    MPI_Request request;
    
    MTest_Init( &argc, &argv );
    
    MPI_Type_contiguous(4,MPI_DOUBLE_INT,&type_ia);
    MPI_Type_commit(&type_ia);
    for (i=1;i<256;i++) omessage[i] = i;
    
    MPI_Isend(omessage, 33, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &request);
    MPI_Recv(imessage, 33, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &recvstatus);
    MPI_Wait(&request, &sendstatus);
    
    /* In versions of MPICH2 through 1.0.7, this call would segfault on
       some platforms (at least on BlueGene).  On others, it would fail
       to report an error.  
       It should return an error (it is invalid to use a different type 
       here than was used in the recv) or at least return MPI_UNDEFINED.
       
       If it returns MPI_UNDEFINED as the count, that will often be
       acceptable.
    */
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    err = MPI_Get_elements(&recvstatus, type_ia, &count);
    if (err == MPI_SUCCESS && count != MPI_UNDEFINED) {
	errs++;
	printf( "MPI_Get_elements did not report an error\n" );
    }

    MPI_Type_free( &type_ia );

    MTest_Finalize( errs );
    MPI_Finalize();

  return 0;
}


