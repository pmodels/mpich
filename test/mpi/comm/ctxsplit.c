/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
 * This check is intended to fail if there is a leak of context ids.  
 * Because this is trying to exhaust the number of context ids, it needs
 * to run for a longer time than many tests.  The for loop uses 100,000 
 * iterations, which is adequate for MPICH2 (with only about 1k context ids
 * available).
 */

main(int argc, char** argv) {

   int i=0;
   int randval;
   int rank;
   int errs = 0;
   MPI_Comm newcomm;

   MTest_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   for (i=0; i<100000; i++) {

     if ( rank == 0 && (i%100 == 0) ) {
       MTestPrintfMsg( 10, "After %d\n", i );
     }

     randval=rand();

     if (randval%(rank+2) == 0) {
       MPI_Comm_split(MPI_COMM_WORLD,1,rank,&newcomm);
       MPI_Comm_free( &newcomm );
     }
     else {
       MPI_Comm_split(MPI_COMM_WORLD,MPI_UNDEFINED,rank,&newcomm);
       if (newcomm != MPI_COMM_NULL) {
	 errs++;
	 printf( "Created a non-null communicator with MPI_UNDEFINED\n" );
       }
     }

   }

   MTest_Finalize( errs );
   MPI_Finalize();

   return 0;
}
