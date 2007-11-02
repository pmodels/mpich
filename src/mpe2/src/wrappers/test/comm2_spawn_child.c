#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    MPI_Comm intercomm;
    char     processor_name[MPI_MAX_PROCESSOR_NAME];
    char     str[10];
    int      rank;
    int      namelen;

    MPI_Init( &argc, &argv );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Get_processor_name( processor_name, &namelen );

    MPI_Comm_get_parent( &intercomm );

    MPI_Recv( str, 4, MPI_CHAR, rank, 0, intercomm, MPI_STATUS_IGNORE );
    printf( "Child %d on %s received from parent: %s\n",
            rank, processor_name, str );
    fflush( stdout );
    MPI_Send( "hi", 3, MPI_CHAR, rank, 0, intercomm );


    MPI_Finalize();
    return 0;
}
