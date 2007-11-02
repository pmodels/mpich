#include <mpi.h>
#include <stdio.h>

	

int
main(
    int					argc,
    char *				argv[])
{
    int					np;
    int					my_rank;
    MPI_Win				win;
    double				my_data;
    double				win_data;
    int					assert;
    
    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    if (np < 2)
    {
	if ( my_rank == 0)
	{
	    printf("\nERROR: fence_acc_simple must be at least (2) "
		   "processes\n\n");
	}
	
	MPI_Finalize();
	return 1;
    }

    if (my_rank == 0)
    {
	MPI_Win_create(&my_data, sizeof(double), sizeof(double), MPI_INFO_NULL,
		       MPI_COMM_WORLD, &win);

	assert = MPI_MODE_NOPRECEDE;
	win_data = 0.0;
    }
    else
    {
	MPI_Win_create(0, 0, 0, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        assert = MPI_MODE_NOSTORE | MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE;
    }

    MPI_Win_fence(assert, win);
    {
	my_data = 42.0;
	MPI_Accumulate(&my_data, 1, MPI_DOUBLE,
		       0, 0, 1, MPI_DOUBLE, MPI_SUM, win);
    }
    MPI_Win_fence(MPI_MODE_NOSTORE | MPI_MODE_NOSUCCEED, win);
	
    if (win_data == 42.0 * np)
    {
	printf("%d: data=%f\n", my_rank, win_data);
    }
    else
    {
	printf("%d: ERROR - data=%f NOT %f\n",
	       my_rank, win_data, np * 42.0);
    }
	
    MPI_Win_free(&win);
	
    MPI_Finalize();
    
    return 0;
}
