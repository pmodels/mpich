/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"

/*
 * This check is intended to fail if there is a leak of context ids.  
 * Because this is trying to exhaust the number of context ids, it needs
 * to run for a longer time than many tests.  The for loop uses 100,000 
 * iterations, which is adequate for MPICH2 (with only about 1k context ids
 * available).
 */

int main(int argc, char** argv) {

   int      i=0;
   int      randval;
   int      rank;
   int      errs = 0;
   MPI_Comm newcomm;
   double   startTime;
   int      nLoop = 100000;
   
   MTest_Init(&argc,&argv);

   for (i=1; i<argc; i++) {
       if (strcmp( argv[i], "--loopcount" ) == 0)  {
	   i++;
	   nLoop = atoi( argv[i] );
       }
       else {
	   fprintf( stderr, "Unrecognized argument %s\n", argv[i] );
       }
   }

   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   startTime = MPI_Wtime();
   for (i=0; i<nLoop; i++) {
       
       if ( rank == 0 && (i%100 == 0) ) {
	   double rate = MPI_Wtime() - startTime;
	   if (rate > 0) {
	       rate = i / rate;
	       MTestPrintfMsg( 10, "After %d (%f)\n", i, rate );
	   }
	   else {
	       MTestPrintfMsg( 10, "After %d\n", i );
	   }
       }
       
       /* FIXME: Explain the rationale behind rand in this test */
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
