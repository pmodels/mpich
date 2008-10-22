#include <mpi.h>

int main( int argc, char *argv[] )
{
  MPI_Comm     comm;
  MPI_Datatype ntype;
  int          i;

  MPI_Init( &argc, &argv );

  for (i=0; i<20; i++) {
    MPI_Comm_dup( MPI_COMM_WORLD, &comm );
  }

  MPI_Type_contiguous( 27, MPI_INT, &ntype );
  MPI_Type_contiguous( 27, MPI_INT, &ntype );
  MPI_Type_contiguous( 27, MPI_INT, &ntype );
  MPI_Type_contiguous( 27, MPI_INT, &ntype );
  MPI_Type_contiguous( 27, MPI_INT, &ntype );
  MPI_Finalize();

  return 0;
}
