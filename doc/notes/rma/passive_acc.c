#include <mpi.h>
#include <stdio.h>

int
main(
    int					argc,
    char *				argv[])
{
    int					np;
    int					rank;
    MPI_Win				win;
    int					data[2];
    
    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    neighbor_rank = (my_rank + 1) % np;
    
    if (np < 2)
    {
	if (rank == 0)
	{
	    printf("\nERROR: fence_get_simple must be at least (2) "
		   "processes\n\n");
	}
	
	MPI_Finalize();
	return 1;
    }

    data[0] = 0;
    data[1] = 0;

    if (rank == 0)
    {
	int				count;
	
	MPI_Win_create(&data, sizeof(int), sizeof(int), MPI_INFO_NULL,
		       MPI_COMM_WORLD, &win);

	do
	{
	    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
	    {
		count = data[0];
	    }
	    MPI_Win_unlock(0, win);
	}
	while (count < np - 1);

	printf("\nPassively accumulated a value of %d from %d processes\n\n",
	       data[1], data[0]);
	
	MPI_Win_free(&win);
    }
    else
    {
	MPI_Win_create(NULL, 0, 0, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

	data[0] = 1;
	data[1] = rank;
	
	MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
	{
	    MPI_Accumulate(data, 2, MPI_INT,
		0, 0, 2, MPI_INT, MPI_SUM, win);
	}
	MPI_Win_unlock(0, win);
    
	MPI_Win_free(&win);
    }

    MPI_Finalize();
    
    return 0;
}
