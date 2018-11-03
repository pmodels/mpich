/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This module tests the message combining algorithm
 * for neighborhood collectives.
 * Currently, only neighbor allgather and neighbor alltoallv
 * fucntions (blocking and nonblocking) are supported.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>
#include "mpitest.h"

#if !defined(USE_STRICT_MPI) && defined(MPICH)
#define TEST_NEIGHB_COLL2 1
#endif

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__);  \
            }                                                                     \
        }                                                                         \
    } while (0)

#define NUM_GRAPHS 10
#define MAX_LAYOUT_NAME_LEN 256

char graph_layout_name[MAX_LAYOUT_NAME_LEN] = { '\0' };

static int make_nhbrhood(int graph_num, MPI_Comm comm,
                         int *indegree_ptr, int **sources_ptr, int **sourcesweights_ptr,
                         int *outdegree_ptr, int **destinations_ptr, int **destweights_ptr)
{
    int my_rank, size;
    MPI_Comm_rank(comm, &my_rank);
    MPI_Comm_size(comm, &size);

    int i, j, indgr, outdgr, inidx, outidx;
    indgr = outdgr = inidx = outidx = 0;

    int *my_row, *my_col;
    my_row = (int *) calloc(size, sizeof(int));
    my_col = (int *) calloc(size, sizeof(int));

    int *contig_vtopo_mat = NULL;
    int *verify_contig_vtopo_mat = NULL;
    int **layout = NULL;

    /* Rank 0 builds the graph and scatters it to others */
    if (my_rank == 0) {
        contig_vtopo_mat = (int *) calloc(size * size, sizeof(int));
        verify_contig_vtopo_mat = (int *) calloc(size * size, sizeof(int));
        layout = (int **) malloc(size * sizeof(int *));
        for (i = 0; i < size; i++)
            layout[i] = &contig_vtopo_mat[i * size];

        /* Making the graph layout -- copied mostly from topo tests */
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
                        if (i == my_rank && j == my_rank)
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

                /* Create a connectivity graph; layout[i,j]==1
                 * represents an outward connectivity from i to j */
                for (i = 0; i < size; i++) {
                    for (j = 0; j < size; j++) {
                        /* disable about a third of the edges */
                        if (((rand() * 1.0) / RAND_MAX) < 0.33)
                            layout[i][j] = 0;
                        else
                            layout[i][j] = 1;
                    }
                }
                break;
        }
    }

    /* Scattering rows of the matrix */
    MPI_Scatter(contig_vtopo_mat, size, MPI_INT, my_row, size, MPI_INT, 0, comm);

    /* Scattering columns of the matrix */
    MPI_Datatype mat_col_t, mat_col_resized_t;
    MPI_Type_vector(size, 1, size, MPI_INT, &mat_col_t);
    MPI_Type_commit(&mat_col_t);
    MPI_Type_create_resized(mat_col_t, 0, 1 * sizeof(int), &mat_col_resized_t);
    MPI_Type_commit(&mat_col_resized_t);
    MPI_Scatter(contig_vtopo_mat, 1, mat_col_resized_t, my_col, size, MPI_INT, 0, comm);

#ifdef ERROR_CHECK
    /* Verify the correctness of matrix distribution */
    MPI_Gather(my_row, size, MPI_INT, verify_contig_vtopo_mat, size, MPI_INT, 0, comm);
    if (my_rank == 0) {
        for (i = 0; i < size * size; i++) {
            if (contig_vtopo_mat[i] != verify_contig_vtopo_mat[i]) {
                fprintf(stderr, "ERROR: contig matrix is not same "
                        "as the aggregation of all my_rows!\n");
                return 1;
            }
        }
        memset(verify_contig_vtopo_mat, 0, size * size * sizeof(int));
    }

    MPI_Gather(my_col, size, MPI_INT, verify_contig_vtopo_mat, 1, mat_col_resized_t, 0, comm);
    if (my_rank == 0) {
        for (i = 0; i < size * size; i++) {
            if (contig_vtopo_mat[i] != verify_contig_vtopo_mat[i]) {
                fprintf(stderr, "ERROR: contig matrix is not "
                        "same as the aggregation of all my_cols!\n");
                return 1;
            }
        }
    }
#endif

    /* free some memory */
    if (my_rank == 0) {
        free(layout);
        free(contig_vtopo_mat);
        free(verify_contig_vtopo_mat);
    }
    MPI_Type_free(&mat_col_resized_t);
    MPI_Type_free(&mat_col_t);

    /* Finding indegree and outdegree */
    for (i = 0; i < size; i++) {
        if (my_row[i] != 0)
            outdgr++;
        if (my_col[i] != 0)
            indgr++;
    }

    int *srcs, *srcwghts, *dests, *destwghts;
    srcs = (int *) malloc(indgr * sizeof(int));
    srcwghts = (int *) malloc(indgr * sizeof(int));
    dests = (int *) malloc(outdgr * sizeof(int));
    destwghts = (int *) malloc(outdgr * sizeof(int));

    for (i = 0; i < indgr; i++)
        srcwghts[i] = 1;
    for (i = 0; i < outdgr; i++)
        destwghts[i] = 1;

    for (i = 0; i < size; i++) {
        if (my_row[i] != 0) {
            dests[outidx] = i;
            outidx++;
        }
        if (my_col[i] != 0) {
            srcs[inidx] = i;
            inidx++;
        }
    }

    /* Returning all values */
    *indegree_ptr = indgr;
    *sources_ptr = srcs;
    *sourcesweights_ptr = srcwghts;
    *outdegree_ptr = outdgr;
    *destinations_ptr = dests;
    *destweights_ptr = destwghts;

    free(my_row);
    free(my_col);

    return 0;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int wrank, wsize;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

#if defined(TEST_NEIGHB_COLL2)

    MPI_Comm dgraph_adj_comm;

    int graph_num;
    for (graph_num = 0; graph_num < NUM_GRAPHS; graph_num++) {
        int indgr, outdgr;
        int *srcs, *srcwghts, *dests, *destwghts;
        int err = make_nhbrhood(graph_num, MPI_COMM_WORLD,
                                &indgr, &srcs, &srcwghts,
                                &outdgr, &dests, &destwghts);
        if (err) {
            MTestError("make_nhbrhood failed! Aborting the test");
        }

        MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD,
                                       indgr, srcs, srcwghts,
                                       outdgr, dests, destwghts,
                                       MPI_INFO_NULL, 0, &dgraph_adj_comm);

        MPI_Errhandler_set(dgraph_adj_comm, MPI_ERRORS_RETURN);

        /* allgather */
        {
            int sendbuf[1] = { wrank };
            int *recvbuf = malloc(indgr * sizeof(int));
            memset(recvbuf, 0xdeadbeef, indgr * sizeof(int));

            MPI_Neighbor_allgather(sendbuf, 1, MPI_INT, recvbuf, 1, MPI_INT, dgraph_adj_comm);

            /* every process should have sent its rank to its outgoing neighbors */
            int i;
            for (i = 0; i < indgr; i++) {
                check(recvbuf[i] == srcs[i]);
            }

            free(recvbuf);
        }

        /* alltoallv */
        {
            /* each rank wrank sends (wrank + dests[i]) % MAX_SENDCOUNTS + 1
             * integers to each of its outgoing neighbors. Each of those
             * integers is equal to (wrank + dests[i]) */
            static const int MAX_SENDCOUNT = 4;
            int *sendcounts = calloc(outdgr, sizeof(int));
            int *recvcounts = calloc(indgr, sizeof(int));
            int *sdispls = calloc(outdgr, sizeof(int));
            int *rdispls = calloc(indgr, sizeof(int));
            int sendcounts_sum = 0;
            int recvcounts_sum = 0;
            int i = 0;
            for (i = 0; i < outdgr; i++) {
                sdispls[i] = sendcounts_sum;
                sendcounts[i] = (wrank + dests[i]) % MAX_SENDCOUNT + 1;
                sendcounts_sum += sendcounts[i];
            }
            for (i = 0; i < indgr; i++) {
                rdispls[i] = recvcounts_sum;
                recvcounts[i] = (wrank + srcs[i]) % MAX_SENDCOUNT + 1;
                recvcounts_sum += recvcounts[i];
            }

            int *sendbuf = malloc(sendcounts_sum * sizeof(int));
            int *recvbuf = malloc(recvcounts_sum * sizeof(int));
            memset(recvbuf, 0xdeadbeef, recvcounts_sum * sizeof(int));

            /* set sendbuf values */
            int j;
            for (i = 0; i < outdgr; i++) {
                int value = wrank + dests[i];
                for (j = 0; j < sendcounts[i]; j++) {
                    sendbuf[sdispls[i] + j] = value;
                }
            }

            MPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, MPI_INT,
                                   recvbuf, recvcounts, rdispls, MPI_INT, dgraph_adj_comm);

            for (i = 0; i < indgr; i++) {
                int value = wrank + srcs[i];
                for (j = 0; j < recvcounts[i]; j++) {
                    check(recvbuf[rdispls[i] + j] == value);
                }
            }

            free(sendbuf);
            free(recvbuf);
            free(sendcounts);
            free(recvcounts);
            free(sdispls);
            free(rdispls);
        }
    }

    MPI_Comm_free(&dgraph_adj_comm);
#endif /* defined(TEST_NEIGHB_COLL2) */

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
