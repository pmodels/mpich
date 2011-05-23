/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_XDIM 1024 
#define MAX_YDIM 1024
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char *argv[]) {

   int i, j, rank, nranks;
   int xdim, ydim;
   long bufsize;
   double **buffer;
   double t_start, t_stop;
   int count[2], src_stride, trg_stride, stride_level, peer;
   double expected, actual;
   int provided;

   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &nranks);

   ARMCI_Init_args(&argc, &argv);
   
   bufsize = MAX_XDIM * MAX_YDIM * sizeof(double);
   buffer = (double **) malloc(sizeof(double *) * nranks);
   ARMCI_Malloc((void **) buffer, bufsize);

   for(i=0; i< bufsize/sizeof(double); i++) {
       *(buffer[rank] + i) = 1.0 + rank;
   }

   if(rank == 0) {
     printf("ARMCI_PutS Latency - local and remote completions - in usec \n");
     printf("%30s %22s %22s\n", "Dimensions(array of doubles)", "Latency-LocalCompeltion", "Latency-RemoteCompletion");
     fflush(stdout);
   }

   src_stride = MAX_YDIM*sizeof(double);
   trg_stride = MAX_YDIM*sizeof(double);
   stride_level = 1;

   ARMCI_Barrier();

   for(xdim=1; xdim<=MAX_XDIM; xdim*=2) {

      count[1] = xdim;

      for(ydim=1; ydim<=MAX_YDIM; ydim*=2) {

        count[0] = ydim*sizeof(double); 
      
        if(rank == 0) 
        {
          peer = 1;          
 
          for(i=0; i<ITERATIONS+SKIP; i++) { 

             if(i == SKIP)
                 t_start = MPI_Wtime();

             ARMCI_PutS((void *) buffer[rank], &src_stride, (void *) buffer[peer], &trg_stride, count, stride_level, peer); 
 
          }
          t_stop = MPI_Wtime();
          ARMCI_Fence(peer);
          char temp[10]; 
          sprintf(temp,"%dX%d", xdim, ydim);
          printf("%30s %20.2f", temp, ((t_stop-t_start)*1000000)/ITERATIONS);
          fflush(stdout);

          ARMCI_Barrier();

          ARMCI_Barrier();

          for(i=0; i<ITERATIONS+SKIP; i++) {
  
             if(i == SKIP)
                t_start = MPI_Wtime();

             ARMCI_PutS((void *) buffer[rank], &src_stride, (void *) buffer[peer], &trg_stride, count, stride_level, peer); 
             ARMCI_Fence(peer);

          }
          t_stop = MPI_Wtime();
          printf("%20.2f \n", ((t_stop-t_start)*1000000)/ITERATIONS);
          fflush(stdout);

          ARMCI_Barrier();

          ARMCI_Barrier();
        }
        else
        {
            peer = 0;

            expected = (1.0 + (double) peer);

            ARMCI_Barrier();
            if (rank == 1)
            {
              for(i=0; i<xdim; i++)
              {
                for(j=0; j<ydim; j++)
                {
                  actual = *(buffer[rank] + i*MAX_YDIM + j);
                  if(actual != expected)
                  {
                    printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                        i, j, expected, actual);
                    fflush(stdout);
                    ARMCI_Error("Bailing out", 1);
                  }
                }
              }
            }
            for(i=0; i< bufsize/sizeof(double); i++) {
              *(buffer[rank] + i) = 1.0 + rank;
            }

            ARMCI_Barrier();

            ARMCI_Barrier();
            if (rank == 1)
            {
              for(i=0; i<xdim; i++)
              {
                for(j=0; j<ydim; j++)
                {
                  actual = *(buffer[rank] + i*MAX_YDIM + j);
                  if(actual != expected)
                  {
                    printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                        i, j, expected, actual);
                    fflush(stdout);
                    ARMCI_Error("Bailing out", 1);
                  }
                }
              }

              for(i=0; i< bufsize/sizeof(double); i++) {
                *(buffer[rank] + i) = 1.0 + rank;
              }
            }
            ARMCI_Barrier();

        }
        
      }

   }

   ARMCI_Barrier();

   ARMCI_Free((void *) buffer[rank]);

   ARMCI_Finalize();

   MPI_Finalize();

   return 0;
}
