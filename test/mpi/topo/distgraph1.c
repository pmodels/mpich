/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mpitest.h"

#define NUM_GRAPHS 10
#define MAX_WEIGHT 100

/* convenience globals */
int size, rank;

/* We need MPI 2.2 to be able to compile the following routines. */
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)

/* Maybe use a bit vector instead? */
int **layout;

#define MAX_LAYOUT_NAME_LEN 256
char graph_layout_name[MAX_LAYOUT_NAME_LEN] = { '\0' };

static void create_graph_layout(int graph_num)
{
    int i, j;

    if (rank == 0) {
        switch (graph_num) {
            case 0:
                strncpy(graph_layout_name, "deterministic complete graph", MAX_LAYOUT_NAME_LEN);
                for (i = 0; i < size; i++)
                    for (j = 0; j < size; j++)
                        layout[i][j] = (i + 2) * (j + 1);
                break;
            case 1:
                strncpy(graph_layout_name, "every other edge deleted", MAX_LAYOUT_NAME_LEN);
                for (i = 0; i < size; i++)
                    for (j = 0; j < size; j++)
                        layout[i][j] = (j % 2 ? (i + 2) * (j + 1) : 0);
                break;
            case 2:
                strncpy(graph_layout_name, "only self-edges", MAX_LAYOUT_NAME_LEN);
                for (i = 0; i < size; i++) {
                    for (j = 0; j < size; j++) {
                        if (i == rank && j == rank)
                            layout[i][j] = 10 * (i + 1);
                        else
                            layout[i][j] = 0;
                    }
                }
                break;
            case 3:
                strncpy(graph_layout_name, "no edges", MAX_LAYOUT_NAME_LEN);
                for (i = 0; i < size; i++)
                    for (j = 0; j < size; j++)
                        layout[i][j] = 0;
                break;
            default:
                strncpy(graph_layout_name, "a random incomplete graph", MAX_LAYOUT_NAME_LEN);
                srand(graph_num);

                /* Create a connectivity graph; layout[i,j]==w represents an outward
                 * connectivity from i to j with weight w, w==0 is no edge. */
                for (i = 0; i < size; i++) {
                    for (j = 0; j < size; j++) {
                        /* disable about a third of the edges */
                        if (((rand() * 1.0) / RAND_MAX) < 0.33)
                            layout[i][j] = 0;
                        else
                            layout[i][j] = rand() % MAX_WEIGHT;
                    }
                }
                break;
        }
    }

    /* because of the randomization we must determine the graph on rank 0 and
     * send the layout to all other processes */
    MPI_Bcast(graph_layout_name, MAX_LAYOUT_NAME_LEN, MPI_CHAR, 0, MPI_COMM_WORLD);
    for (i = 0; i < size; ++i) {
        MPI_Bcast(layout[i], size, MPI_INT, 0, MPI_COMM_WORLD);
    }
}

static int verify_comm(MPI_Comm comm)
{
    int local_errs = 0;
    int i, j;
    int indegree, outdegree, weighted;
    int *sources, *sweights, *destinations, *dweights;
    int use_dup;
    int topo_type = MPI_UNDEFINED;
    MPI_Comm dupcomm = MPI_COMM_NULL;

    sources = (int *) malloc(size * sizeof(int));
    sweights = (int *) malloc(size * sizeof(int));
    destinations = (int *) malloc(size * sizeof(int));
    dweights = (int *) malloc(size * sizeof(int));

    for (use_dup = 0; use_dup <= 1; ++use_dup) {
        if (!use_dup) {
            MPI_Dist_graph_neighbors_count(comm, &indegree, &outdegree, &weighted);
        } else {
            MPI_Comm_dup(comm, &dupcomm);
            comm = dupcomm;     /* caller retains original comm value */
        }

        MPI_Topo_test(comm, &topo_type);
        if (topo_type != MPI_DIST_GRAPH) {
            fprintf(stderr, "topo_type != MPI_DIST_GRAPH\n");
            ++local_errs;
        }

        j = 0;
        for (i = 0; i < size; i++)
            if (layout[i][rank])
                j++;
        if (j != indegree) {
            fprintf(stderr, "indegree does not match, expected=%d got=%d, layout='%s'\n", indegree,
                    j, graph_layout_name);
            ++local_errs;
        }

        j = 0;
        for (i = 0; i < size; i++)
            if (layout[rank][i])
                j++;
        if (j != outdegree) {
            fprintf(stderr, "outdegree does not match, expected=%d got=%d, layout='%s'\n",
                    outdegree, j, graph_layout_name);
            ++local_errs;
        }

        if ((indegree || outdegree) && (weighted == 0)) {
            fprintf(stderr, "MPI_Dist_graph_neighbors_count thinks the graph is not weighted\n");
            ++local_errs;
        }


        MPI_Dist_graph_neighbors(comm, indegree, sources, sweights, outdegree, destinations,
                                 dweights);

        /* For each incoming and outgoing edge in the matrix, search if
         * the query function listed it in the sources. */
        for (i = 0; i < size; i++) {
            if (layout[i][rank]) {
                for (j = 0; j < indegree; j++) {
                    assert(sources[j] >= 0);
                    assert(sources[j] < size);
                    if (sources[j] == i)
                        break;
                }
                if (j == indegree) {
                    fprintf(stderr, "no edge from %d to %d specified\n", i, rank);
                    ++local_errs;
                } else {
                    if (sweights[j] != layout[i][rank]) {
                        fprintf(stderr, "incorrect weight for edge (%d,%d): %d instead of %d\n",
                                i, rank, sweights[j], layout[i][rank]);
                        ++local_errs;
                    }
                }
            }
            if (layout[rank][i]) {
                for (j = 0; j < outdegree; j++) {
                    assert(destinations[j] >= 0);
                    assert(destinations[j] < size);
                    if (destinations[j] == i)
                        break;
                }
                if (j == outdegree) {
                    fprintf(stderr, "no edge from %d to %d specified\n", rank, i);
                    ++local_errs;
                } else {
                    if (dweights[j] != layout[rank][i]) {
                        fprintf(stderr, "incorrect weight for edge (%d,%d): %d instead of %d\n",
                                rank, i, dweights[j], layout[rank][i]);
                        ++local_errs;
                    }
                }
            }
        }

        /* For each incoming and outgoing edge in the sources, we should
         * have an entry in the matrix */
        for (i = 0; i < indegree; i++) {
            if (layout[sources[i]][rank] != sweights[i]) {
                fprintf(stderr, "edge (%d,%d) has a weight %d instead of %d\n", i, rank,
                        sweights[i], layout[sources[i]][rank]);
                ++local_errs;
            }
        }
        for (i = 0; i < outdegree; i++) {
            if (layout[rank][destinations[i]] != dweights[i]) {
                fprintf(stderr, "edge (%d,%d) has a weight %d instead of %d\n", rank, i,
                        dweights[i], layout[rank][destinations[i]]);
                ++local_errs;
            }
        }
    }

    if (dupcomm != MPI_COMM_NULL)
        MPI_Comm_free(&dupcomm);

    free(sources);
    free(sweights);
    free(destinations);
    free(dweights);
    return local_errs;
}

#endif /* At least MPI 2.2 */

int main(int argc, char *argv[])
{
    int errs = 0;
    int i, j, k, p;
    int indegree, outdegree, reorder;
    int check_indegree, check_outdegree, check_weighted;
    int *sources, *sweights, *destinations, *dweights, *degrees;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    layout = (int **) malloc(size * sizeof(int *));
    assert(layout);
    for (i = 0; i < size; i++) {
        layout[i] = (int *) malloc(size * sizeof(int));
        assert(layout[i]);
    }
    /* alloc size*size ints to handle the all-on-one-process case */
    sources = (int *) malloc(size * size * sizeof(int));
    sweights = (int *) malloc(size * size * sizeof(int));
    destinations = (int *) malloc(size * size * sizeof(int));
    dweights = (int *) malloc(size * size * sizeof(int));
    degrees = (int *) malloc(size * size * sizeof(int));

    for (i = 0; i < NUM_GRAPHS; i++) {
        create_graph_layout(i);
        if (rank == 0) {
            MTestPrintfMsg(1, "using graph layout '%s'\n", graph_layout_name);
        }

        /* MPI_Dist_graph_create_adjacent */
        if (rank == 0) {
            MTestPrintfMsg(1, "testing MPI_Dist_graph_create_adjacent\n");
        }
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
            errs += verify_comm(comm);
            MPI_Comm_free(&comm);
        }

        /* a weak check that passing MPI_UNWEIGHTED doesn't cause
         * create_adjacent to explode */
        MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, indegree, sources, MPI_UNWEIGHTED,
                                       outdegree, destinations, MPI_UNWEIGHTED, MPI_INFO_NULL,
                                       reorder, &comm);
        MPI_Barrier(comm);
        /* intentionally no verify here, weights won't match */
        MPI_Comm_free(&comm);


        /* MPI_Dist_graph_create() where each process specifies its
         * outgoing edges */
        if (rank == 0) {
            MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ outgoing only\n");
        }
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
            errs += verify_comm(comm);
            MPI_Comm_free(&comm);
        }


        /* MPI_Dist_graph_create() where each process specifies its
         * incoming edges */
        if (rank == 0) {
            MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ incoming only\n");
        }
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
            MPI_Dist_graph_create(MPI_COMM_WORLD, k, sources, degrees, destinations, sweights,
                                  MPI_INFO_NULL, reorder, &comm);
            MPI_Barrier(comm);
            errs += verify_comm(comm);
            MPI_Comm_free(&comm);
        }


        /* MPI_Dist_graph_create() where rank 0 specifies the entire
         * graph */
        if (rank == 0) {
            MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ rank 0 specifies only\n");
        }
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
            errs += verify_comm(comm);
            MPI_Comm_free(&comm);
        }

        /* MPI_Dist_graph_create() where rank 0 specifies the entire
         * graph and all other ranks pass NULL.  Can catch implementation
         * problems when MPI_UNWEIGHTED==NULL. */
        if (rank == 0) {
            MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ rank 0 specifies only -- NULLs\n");
        }
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
            if (rank == 0) {
                MPI_Dist_graph_create(MPI_COMM_WORLD, p, sources, degrees,
                                      destinations, sweights, MPI_INFO_NULL, reorder, &comm);
            } else {
                MPI_Dist_graph_create(MPI_COMM_WORLD, 0, NULL, NULL,
                                      NULL, NULL, MPI_INFO_NULL, reorder, &comm);
            }
            MPI_Barrier(comm);
            errs += verify_comm(comm);
            MPI_Comm_free(&comm);
        }
    }

    /* now tests that don't depend on the layout[][] array */

    /* The MPI-2.2 standard recommends implementations set
     * MPI_UNWEIGHTED==NULL, but this leads to an ambiguity.  The draft
     * MPI-3.0 standard specifically recommends _not_ setting it equal
     * to NULL. */
    if (MPI_UNWEIGHTED == NULL) {
        fprintf(stderr, "MPI_UNWEIGHTED should not be NULL\n");
        ++errs;
    }

    /* MPI_Dist_graph_create() with no graph */
    if (rank == 0) {
        MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ no graph\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create(MPI_COMM_WORLD, 0, sources, degrees,
                              destinations, sweights, MPI_INFO_NULL, reorder, &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        if (!check_weighted) {
            fprintf(stderr, "expected weighted == TRUE for the \"no graph\" case\n");
            ++errs;
        }
        MPI_Comm_free(&comm);
    }

    /* MPI_Dist_graph_create() with no graph -- passing MPI_WEIGHTS_EMPTY
     * instead */
    /* NOTE that MPI_WEIGHTS_EMPTY was added in MPI-3 and does not
     * appear before then.  This part of the test thus requires a check
     * on the MPI major version */
#if MPI_VERSION >= 3
    if (rank == 0) {
        MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ no graph\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create(MPI_COMM_WORLD, 0, sources, degrees,
                              destinations, MPI_WEIGHTS_EMPTY, MPI_INFO_NULL, reorder, &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        if (!check_weighted) {
            fprintf(stderr,
                    "expected weighted == TRUE for the \"no graph -- MPI_WEIGHTS_EMPTY\" case\n");
            ++errs;
        }
        MPI_Comm_free(&comm);
    }
#endif

    /* MPI_Dist_graph_create() with no graph -- passing NULLs instead */
    if (rank == 0) {
        MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ no graph -- NULLs\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create(MPI_COMM_WORLD, 0, NULL, NULL,
                              NULL, NULL, MPI_INFO_NULL, reorder, &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        /* ambiguous if they are equal, only check when they are distinct values. */
        if (MPI_UNWEIGHTED != NULL) {
            if (!check_weighted) {
                fprintf(stderr, "expected weighted == TRUE for the \"no graph -- NULLs\" case\n");
                ++errs;
            }
        }
        MPI_Comm_free(&comm);
    }

    /* MPI_Dist_graph_create() with no graph -- passing NULLs+MPI_UNWEIGHTED instead */
    if (rank == 0) {
        MTestPrintfMsg(1, "testing MPI_Dist_graph_create w/ no graph -- NULLs+MPI_UNWEIGHTED\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create(MPI_COMM_WORLD, 0, NULL, NULL,
                              NULL, MPI_UNWEIGHTED, MPI_INFO_NULL, reorder, &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        /* ambiguous if they are equal, only check when they are distinct values. */
        if (MPI_UNWEIGHTED != NULL) {
            if (check_weighted) {
                fprintf(stderr,
                        "expected weighted == FALSE for the \"no graph -- NULLs+MPI_UNWEIGHTED\" case\n");
                ++errs;
            }
        }
        MPI_Comm_free(&comm);
    }

    /* MPI_Dist_graph_create_adjacent() with no graph */
    if (rank == 0) {
        MTestPrintfMsg(1, "testing MPI_Dist_graph_create_adjacent w/ no graph\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, 0, sources, sweights,
                                       0, destinations, dweights, MPI_INFO_NULL, reorder, &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        if (!check_weighted) {
            fprintf(stderr, "expected weighted == TRUE for the \"no graph\" case\n");
            ++errs;
        }
        MPI_Comm_free(&comm);
    }

    /* MPI_Dist_graph_create_adjacent() with no graph -- passing MPI_WEIGHTS_EMPTY instead */
    /* NOTE that MPI_WEIGHTS_EMPTY was added in MPI-3 and does not
     * appear before then.  This part of the test thus requires a check
     * on the MPI major version */
#if MPI_VERSION >= 3
    if (rank == 0) {
        MTestPrintfMsg(1,
                       "testing MPI_Dist_graph_create_adjacent w/ no graph -- MPI_WEIGHTS_EMPTY\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, 0, sources, MPI_WEIGHTS_EMPTY,
                                       0, destinations, MPI_WEIGHTS_EMPTY, MPI_INFO_NULL, reorder,
                                       &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        if (!check_weighted) {
            fprintf(stderr,
                    "expected weighted == TRUE for the \"no graph -- MPI_WEIGHTS_EMPTY\" case\n");
            ++errs;
        }
        MPI_Comm_free(&comm);
    }
#endif

    /* MPI_Dist_graph_create_adjacent() with no graph -- passing NULLs instead */
    if (rank == 0) {
        MTestPrintfMsg(1, "testing MPI_Dist_graph_create_adjacent w/ no graph -- NULLs\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, 0, NULL, NULL,
                                       0, NULL, NULL, MPI_INFO_NULL, reorder, &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        /* ambiguous if they are equal, only check when they are distinct values. */
        if (MPI_UNWEIGHTED != NULL) {
            if (!check_weighted) {
                fprintf(stderr, "expected weighted == TRUE for the \"no graph -- NULLs\" case\n");
                ++errs;
            }
        }
        MPI_Comm_free(&comm);
    }

    /* MPI_Dist_graph_create_adjacent() with no graph -- passing NULLs+MPI_UNWEIGHTED instead */
    if (rank == 0) {
        MTestPrintfMsg(1,
                       "testing MPI_Dist_graph_create_adjacent w/ no graph -- NULLs+MPI_UNWEIGHTED\n");
    }
    for (reorder = 0; reorder <= 1; reorder++) {
        MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, 0, NULL, MPI_UNWEIGHTED,
                                       0, NULL, MPI_UNWEIGHTED, MPI_INFO_NULL, reorder, &comm);
        MPI_Dist_graph_neighbors_count(comm, &check_indegree, &check_outdegree, &check_weighted);
        /* ambiguous if they are equal, only check when they are distinct values. */
        if (MPI_UNWEIGHTED != NULL) {
            if (check_weighted) {
                fprintf(stderr,
                        "expected weighted == FALSE for the \"no graph -- NULLs+MPI_UNWEIGHTED\" case\n");
                ++errs;
            }
        }
        MPI_Comm_free(&comm);
    }


    for (i = 0; i < size; i++)
        free(layout[i]);
    free(layout);
    free(sources);
    free(sweights);
    free(destinations);
    free(dweights);
    free(degrees);
#endif

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
