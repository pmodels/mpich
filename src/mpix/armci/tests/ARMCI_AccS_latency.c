/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_XDIM 1024 
#define MAX_YDIM 1024
#define ITERATIONS 10
#define SKIP 1

#ifdef VANILLA_ARMCI
#define ARMCI_Access_begin(X) ((void*)0)
#define ARMCI_Access_end(X) ((void*)0)
#endif 

int main(int argc, char **argv)
{

    int i, j, rank, nranks, peer;
    size_t xdim, ydim;
    unsigned long bufsize;
    double **buffer, *src_buf;
    double t_start, t_stop;
    int count[2], src_stride, trg_stride, stride_level;
    double scaling;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    buffer = (double **) malloc(sizeof(double *) * nranks);

    bufsize = MAX_XDIM * MAX_YDIM * sizeof(double);
    ARMCI_Malloc((void **) buffer, bufsize);
    src_buf = ARMCI_Malloc_local(bufsize);

    if (rank == 0)
    {
        printf("ARMCI_AccS Latency - local and remote completions - in usec \n");
        printf("%30s %22s %22s\n",
               "Dimensions(array of double)",
               "Local Completion",
               "Remote completion");
        fflush(stdout);
    }

    ARMCI_Access_begin(buffer[rank]);
    for (i = 0; i < bufsize / sizeof(double); i++)
    {
      *(buffer[rank] + i) = 1.0 + rank;
      *(src_buf + i) = 1.0 + rank;
    }
    ARMCI_Access_end(buffer[rank]);

    scaling = 2.0;

    src_stride = MAX_YDIM * sizeof(double);
    trg_stride = MAX_YDIM * sizeof(double);
    stride_level = 1;

    ARMCI_Barrier();

    for (xdim = 1; xdim <= MAX_XDIM; xdim *= 2)
    {

        count[1] = xdim;

        for (ydim = 1; ydim <= MAX_YDIM; ydim *= 2)
        {

            count[0] = ydim * sizeof(double);

            if (rank == 0)
            {

                peer = 1;

                for (i = 0; i < ITERATIONS + SKIP; i++)
                {

                    if (i == SKIP) t_start = MPI_Wtime();

                    ARMCI_AccS(ARMCI_ACC_DBL,
                               (void *) &scaling,
                               /* (void *) buffer[rank] */ src_buf,
                               &src_stride,
                               (void *) buffer[peer],
                               &trg_stride,
                               count,
                               stride_level,
                               1);

                }
                t_stop = MPI_Wtime();
                ARMCI_Fence(1);

                char temp[10];
                sprintf(temp, "%dX%d", (int) xdim, (int) ydim);
                printf("%30s %20.2f ", temp, ((t_stop - t_start) * 1000000)
                        / ITERATIONS);
                fflush(stdout);

                ARMCI_Barrier();

                ARMCI_Barrier();

                for (i = 0; i < ITERATIONS + SKIP; i++)
                {

                    if (i == SKIP) t_start = MPI_Wtime();

                    ARMCI_AccS(ARMCI_ACC_DBL,
                               (void *) &scaling,
                               /* (void *) buffer[rank] */ src_buf,
                               &src_stride,
                               (void *) buffer[peer],
                               &trg_stride,
                               count,
                               stride_level,
                               1);
                    ARMCI_Fence(1);

                }
                t_stop = MPI_Wtime();
                printf("%20.2f \n", ((t_stop - t_start) * 1000000) / ITERATIONS);
                fflush(stdout);

                ARMCI_Barrier();

                ARMCI_Barrier();

            }
            else
            {

                peer = 0;

                ARMCI_Barrier();

                if (rank == 1) 
                {
                  ARMCI_Access_begin(buffer[rank]);
                  for (i = 0; i < xdim; i++)
                  {
                    for (j = 0; j < ydim; j++)
                    {
                      if (*(buffer[rank] + i * MAX_XDIM + j) != ((1.0 + rank)
                            + scaling * (1.0 + peer) * (ITERATIONS + SKIP)))
                      {
                        printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                            i,
                            j,
                            ((1.0 + rank) + scaling * (1.0 + peer)),
                            *(buffer[rank] + i * MAX_YDIM + j));
                        fflush(stdout);
                        ARMCI_Error("Bailing out", 1);
                      }
                    }
                  }

                  for (i = 0; i < bufsize / sizeof(double); i++)
                  {
                    *(buffer[rank] + i) = 1.0 + rank;
                  }
                  ARMCI_Access_end(buffer[rank]);
                }

                ARMCI_Barrier();

                ARMCI_Barrier();

                if (rank == 1) 
                {
                  ARMCI_Access_begin(buffer[rank]);

                  for (i = 0; i < xdim; i++)
                  {
                    for (j = 0; j < ydim; j++)
                    {
                      if (*(buffer[rank] + i * MAX_XDIM + j) != ((1.0 + rank)
                            + scaling * (1.0 + peer) * (ITERATIONS + SKIP)))
                      {
                        printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                            i,
                            j,
                            ((1.0 + rank) + scaling * (1.0 + peer)),
                            *(buffer[rank] + i * MAX_YDIM + j));
                        fflush(stdout);
                        ARMCI_Error("Bailing out", 1);
                      }
                    }
                  }

                  for (i = 0; i < bufsize / sizeof(double); i++)
                  {
                    *(buffer[rank] + i) = 1.0 + rank;
                  }

                  ARMCI_Access_end(buffer[rank]);
                }
                ARMCI_Barrier();

            }

        }

    }

    ARMCI_Barrier();

    ARMCI_Free((void *) buffer[rank]);
    ARMCI_Free_local(src_buf);

    ARMCI_Finalize();

    MPI_Finalize();

    return 0;
}
