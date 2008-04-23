#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* Test that comm_split fails gracefully when the number of communicators
   is exhausted.  Original version submitted by Paul Sack.
*/
#define MAX_COMMS 2000

/* Define VERBOSE to get an output line for every communicator */
/* #define VERBOSE 1 */
int main(int argc, char* argv[])
{
  int i, rank;
  int rc, maxcomm;
  MPI_Comm comm[MAX_COMMS];

  MPI_Init (&argc, &argv);
  MPI_Comm_rank ( MPI_COMM_WORLD, &rank );
  MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

  maxcomm = -1;
  for (i = 0; i < MAX_COMMS; i++) {
#ifdef VERBOSE
    if (rank == 0) {
      if (i % 20 == 0) {
	fprintf(stderr, "\n %d: ", i); fflush(stdout);
      }
      else {
	fprintf(stderr, "."); fflush(stdout);
      }
    }
#endif
    rc = MPI_Comm_split(MPI_COMM_WORLD, 1, rank, &comm[i]);
    if (rc != MPI_SUCCESS) {
      break;
    }
    maxcomm = i;
  }
  for (i=0; i<=maxcomm; i++) {
    MPI_Comm_free( &comm[i] );
  }
  MPI_Finalize();
  return 0;
}
