/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h" 
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"

/* Fetch and add example from Using MPI-2 (the non-scalable version,
   Fig. 6.12). */ 

/* same as fetchandadd.c but uses alloc_mem */

#define NTIMES 20  /* no of times each process calls the counter
                      routine */

int localvalue=0;  /* contribution of this process to the counter. We
                    define it as a global variable because attribute
                    caching on the window is not enabled yet. */ 

void Get_nextval(MPI_Win win, int *val_array, MPI_Datatype get_type,
                 int rank, int nprocs, int *value);

int compar(const void *a, const void *b);

int main(int argc, char *argv[]) 
{ 
    int rank, nprocs, i, blens[2], disps[2], *counter_mem, *val_array,
        *results, *counter_vals;
    MPI_Datatype get_type;
    MPI_Win win;
    int errs = 0;
 
    MTest_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

    if (rank == 0) {
        /* allocate counter memory and initialize to 0 */
        /* counter_mem = (int *) calloc(nprocs, sizeof(int)); */

        i = MPI_Alloc_mem(nprocs*sizeof(int), MPI_INFO_NULL, &counter_mem);
        if (i) {
            printf("Can't allocate memory in test program\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (i=0; i<nprocs; i++) counter_mem[i] = 0;

        MPI_Win_create(counter_mem, nprocs*sizeof(int), sizeof(int),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        MPI_Win_free(&win); 
        MPI_Free_mem(counter_mem);

        /* gather the results from other processes, sort them, and check 
           whether they represent a counter being incremented by 1 */

        results = (int *) malloc(NTIMES*nprocs*sizeof(int));
        for (i=0; i<NTIMES*nprocs; i++)
            results[i] = -1;

        MPI_Gather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, results, NTIMES, MPI_INT, 
                   0, MPI_COMM_WORLD);

        qsort(results+NTIMES, NTIMES*(nprocs-1), sizeof(int), compar);

        for (i=NTIMES+1; i<(NTIMES*nprocs); i++)
            if (results[i] != results[i-1] + 1)
                errs++;
        
        free(results);
    }
    else {
        blens[0] = rank;
        disps[0] = 0;
        blens[1] = nprocs - rank - 1;
        disps[1] = rank + 1;

        MPI_Type_indexed(2, blens, disps, MPI_INT, &get_type);
        MPI_Type_commit(&get_type);

        val_array = (int *) malloc(nprocs * sizeof(int));

        /* allocate array to store the values obtained from the 
           fetch-and-add counter */
        counter_vals = (int *) malloc(NTIMES * sizeof(int));

        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win); 

        for (i=0; i<NTIMES; i++) {
            Get_nextval(win, val_array, get_type, rank, nprocs, counter_vals+i);
            /* printf("Rank %d, counter %d\n", rank, value); */
        }

        MPI_Win_free(&win);

        free(val_array);
        MPI_Type_free(&get_type);

        /* gather the results to the root */
        MPI_Gather(counter_vals, NTIMES, MPI_INT, NULL, 0, MPI_DATATYPE_NULL, 
                   0, MPI_COMM_WORLD);
        free(counter_vals);
    }

    MTest_Finalize(errs);
    MPI_Finalize(); 
    return 0; 
} 


void Get_nextval(MPI_Win win, int *val_array, MPI_Datatype get_type,
                 int rank, int nprocs, int *value) 
{
    int one=1, i;

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
    MPI_Accumulate(&one, 1, MPI_INT, 0, rank, 1, MPI_INT, MPI_SUM, win);
    MPI_Get(val_array, 1, get_type, 0, 0, 1, get_type, win); 
    MPI_Win_unlock(0, win);

    *value = 0;
    val_array[rank] = localvalue;
    for (i=0; i<nprocs; i++)
        *value = *value + val_array[i];

    localvalue++;
}

int compar(const void *a, const void *b)
{
    return (*((int *)a) - *((int *)b));
}

