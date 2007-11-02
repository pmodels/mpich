#include <mpi.h>
#include <stdio.h>

	

int
main(
    int					argc,
    char *				argv[])
{
    int					np;
    int					my_rank;
    int					neighbor_rank;
    MPI_Win				win;
    double				my_data;
    double				neighbor_data;
    
    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    neighbor_rank = (my_rank + 1) % np;
    
    if (np < 2)
    {
	if ( my_rank == 0)
	{
	    printf("\nERROR: fence_put_simple must be at least (2) "
		   "processes\n\n");
	}
	
	MPI_Finalize();
	return 1;
    }

    MPI_Win_create(&my_data, sizeof(double), sizeof(double), MPI_INFO_NULL,
		   MPI_COMM_WORLD, &win);
    
	
    MPI_Win_fence(MPI_MODE_NOSTORE | MPI_MODE_NOPRECEDE, win);
    {
	neighbor_data = 42.0 * neighbor_rank;
	MPI_Put(&neighbor_data, 1, MPI_DOUBLE,
		neighbor_rank, 0, 1, MPI_DOUBLE, win);
    }
    MPI_Win_fence(MPI_MODE_NOSTORE | MPI_MODE_NOSUCCEED, win);
	
    if (my_data == 42.0 * my_rank)
    {
	printf("%d: data[%d]=%f\n", my_rank, my_rank, my_data);
    }
    else
    {
	printf("%d: ERROR - data[%d]=%f NOT %f\n",
	       my_rank, my_rank, my_data, my_rank * 42.0);
    }
	
    MPI_Win_free(&win);

    MPI_Finalize();
    
    return 0;
}
