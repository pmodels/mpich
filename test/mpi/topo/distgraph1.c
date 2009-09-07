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
int **layout, size, rank;

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

static void verify_comm(MPI_Comm comm)
{
    int i, j;
    int indegree, outdegree, weighted;
    int *sources, *sweights, *destinations, *dweights;

    sources = (int *) malloc(size * sizeof(int));
    sweights = (int *) malloc(size * sizeof(int));
    destinations = (int *) malloc(size * sizeof(int));
    dweights = (int *) malloc(size * sizeof(int));

    MPI_Dist_graph_neighbors_count(comm, &indegree, &outdegree, &weighted);

    j = 0;
    for (i = 0; i < size; i++)
        if (layout[i][rank])
            j++;
    if (j != indegree)
        fprintf(stderr, "indegree does not match\n");

    j = 0;
    for (i = 0; i < size; i++)
        if (layout[rank][i])
            j++;
    if (j != outdegree)
        fprintf(stderr, "outdegree does not match\n");

    if (weighted == 0)
        fprintf(stderr, "MPI_Dist_graph_neighbors_count thinks the graph is not weighted\n");


    MPI_Dist_graph_neighbors(comm, size, sources, sweights, size, destinations, dweights);

    /* For each incoming and outgoing edge in the matrix, search if
     * the query function listed it in the sources. */
    for (i = 0; i < size; i++) {
        if (layout[i][rank]) {
            for (j = 0; j < indegree; j++) {
                if (sources[j] == i)
                    break;
            }
            if (sources[j] != i)
                fprintf(stderr, "no edge from %d to %d specified\n", i, rank);
            if (sweights[j] != layout[i][rank])
                fprintf(stderr, "incorrect weight for edge (%d,%d): %d instead of %d\n",
                        i, rank, sweights[j], layout[i][rank]);
        }
        if (layout[rank][i]) {
            for (j = 0; j < outdegree; j++) {
                if (destinations[j] == i)
                    break;
            }
            if (destinations[j] != i)
                fprintf(stderr, "no edge from %d to %d specified\n", rank, i);
            if (dweights[j] != layout[rank][i])
                fprintf(stderr, "incorrect weight for edge (%d,%d): %d instead of %d\n",
                        rank, i, dweights[j], layout[rank][i]);
        }
    }

    /* For each incoming and outgoing edge in the sources, we should
     * have an entry in the matrix */
    for (i = 0; i < indegree; i++) {
        if (layout[sources[i]][rank] != sweights[i])
            fprintf(stderr, "edge (%d,%d) has a weight %d instead of %d", i, rank,
                    sweights[i], layout[sources[i]][rank]);
    }
    for (i = 0; i < outdegree; i++) {
        if (layout[rank][destinations[i]] != dweights[i])
            fprintf(stderr, "edge (%d,%d) has a weight %d instead of %d", rank, i,
                    dweights[i], layout[rank][destinations[i]]);
    }
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int i, j, k, p;
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

        /* MPI_Dist_graph_create_adjacent */
        indegree = 0;
        k = 0;
        for (j = 0; j < size; j++) {
            if (layout[j][rank]) {
                indegree++;
                sources[k] = j;
                sweights[k++] = layout[j][rank];
            }
        }

        outdegree = 0;
        k = 0;
        for (j = 0; j < size; j++) {
            if (layout[rank][j]) {
                outdegree++;
                destinations[k] = j;
                dweights[k++] = layout[rank][j];
            }
        }

        for (reorder = 0; reorder <= 1; reorder++) {
            MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, indegree, sources, sweights,
                                           outdegree, destinations, dweights, MPI_INFO_NULL,
                                           reorder, &comm);
            MPI_Barrier(comm);
            MPI_Comm_free(&comm);
        }

        verify_comm(comm);


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

        verify_comm(comm);

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

        verify_comm(comm);

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

        verify_comm(comm);

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
