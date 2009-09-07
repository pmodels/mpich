/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpitest.h"

#define NUM_GRAPHS 10
#define MAX_WEIGHT 100

/* Maybe use a bit vector instead? */
int **layout, size;

static void create_graph_layout(int seed)
{
    int i, j;

    srand(seed);

    /* Create a connectivity graph; 1 in [i,j] represents an outward
     * connectivity from i to j. */
    for (i = 0; i < size; i++)
        for (j = 0; j < size; j++)
            layout[i][j] = rand() % MAX_WEIGHT;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, i, j, k, p;
    int indegree, outdegree, reorder;
    int *sources, *sweights, *destinations, *dweights, *degrees;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    layout = (int **) malloc(size * sizeof(int *));
    assert(layout);
    for (i = 0; i < size; i++) {
        layout[i] = (int *) malloc(size * sizeof(int));
        assert(layout[i]);
    }
    sources = (int *) malloc(size * sizeof(int));
    sweights = (int *) malloc(size * sizeof(int));
    destinations = (int *) malloc(size * sizeof(int));
    dweights = (int *) malloc(size * sizeof(int));
    degrees = (int *) malloc(size * sizeof(int));

    for (i = 0; i < NUM_GRAPHS; i++) {
        create_graph_layout(i);

        indegree = 0;
        k = 0;
        for (j = 0; j < size; j++)
            if (layout[j][rank]) {
                indegree++;
                sources[k] = j;
                sweights[k++] = layout[j][rank];
            }

        outdegree = 0;
        k = 0;
        for (j = 0; j < size; j++)
            if (layout[rank][j]) {
                outdegree++;
                destinations[k] = j;
                dweights[k++] = layout[rank][j];
            }

        for (reorder = 0; reorder <= 1; reorder++) {
            MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, indegree, sources, sweights,
                                           outdegree, destinations, dweights, MPI_INFO_NULL,
                                           reorder, &comm);
            MPI_Barrier(comm);
            MPI_Comm_free(&comm);
        }

        /* MPI_Dist_graph_create() where each process specifies its
         * outgoing edges */
        sources[0] = rank;
        k = 0;
        for (j = 0; j < size; j++) {
            if (layout[rank][j]) {
                destinations[k] = j;
                dweights[k++] = layout[rank][j];
            }
        }
        degrees[0] = k;
        for (reorder = 0; reorder <= 1; reorder++) {
            MPI_Dist_graph_create(MPI_COMM_WORLD, 1, sources, degrees, destinations, dweights,
                                  MPI_INFO_NULL, reorder, &comm);
            MPI_Barrier(comm);
            MPI_Comm_free(&comm);
        }

        /* MPI_Dist_graph_create() where each process specifies its
         * incoming edges */
        k = 0;
        for (j = 0; j < size; j++) {
            if (layout[j][rank]) {
                sources[k] = j;
                sweights[k] = layout[j][rank];
                degrees[k] = 1;
                destinations[k++] = rank;
            }
        }
        for (reorder = 0; reorder <= 1; reorder++) {
            MPI_Dist_graph_create(MPI_COMM_WORLD, 1, sources, degrees, destinations, sweights,
                                  MPI_INFO_NULL, reorder, &comm);
            MPI_Barrier(comm);
            MPI_Comm_free(&comm);
        }

        /* MPI_Dist_graph_create() where rank 0 specifies the entire
         * graph */
        p = 0;
        for (j = 0; j < size; j++) {
            for (k = 0; k < size; k++) {
                if (layout[j][k]) {
                    sources[p] = j;
                    sweights[p] = layout[j][k];
                    degrees[p] = 1;
                    destinations[p++] = k;
                }
            }
        }
        for (reorder = 0; reorder <= 1; reorder++) {
            MPI_Dist_graph_create(MPI_COMM_WORLD, (rank == 0) ? p : 0, sources, degrees,
                                  destinations, sweights, MPI_INFO_NULL, reorder, &comm);
            MPI_Barrier(comm);
            MPI_Comm_free(&comm);
        }

        /* MPI_Dist_graph_create() with no graph */
        for (reorder = 0; reorder <= 1; reorder++) {
            MPI_Dist_graph_create(MPI_COMM_WORLD, 0, sources, degrees,
                                  destinations, sweights, MPI_INFO_NULL, reorder, &comm);
            MPI_Comm_free(&comm);
        }
    }

    for (i = 0; i < size; i++)
        free(layout[i]);
    free(layout);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
