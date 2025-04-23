/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

/* This test divide 16 processes into 4 groups (nodes), each group
 * creates a node_comm. Then roots of each node group creates a roots_comm.
 */

#define NP 16
#define PPN 4
#define N 4

int main(int argc, char *argv[])
{
    int errs = 0;

    int ret;
    MPI_Session session;
    MPI_Group world_group;
    MPI_Comm comm;

    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &session);

    MPI_Group_from_session_pset(session, "mpi://world", &world_group);

    int world_rank, world_size;
    MPI_Group_rank(world_group, &world_rank);
    MPI_Group_size(world_group, &world_size);

    if (world_size != NP) {
        printf("This test require %d processes, world_size is %d.\n", NP, world_size);
        goto fn_exit;
    }
    if (N * PPN != NP) {
        printf("N(%d) x PPN(%d) != NP(%d)\n", N, PPN, NP);
        goto fn_exit;
    }

    MPI_Group node_group, roots_group;
    MPI_Comm node_comm, roots_comm;

    int proc_list[NP];
    int node_root = (world_rank / PPN) * PPN;
    int is_root = (world_rank == node_root);

    for (int i = 0; i < PPN; i++) {
        proc_list[i] = node_root + i;
    }
    MPI_Group_incl(world_group, PPN, proc_list, &node_group);
    MPI_Comm_create_from_group(node_group, "tag", MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &node_comm);

    if (is_root) {
        for (int i = 0; i < N; i++) {
            proc_list[i] = i * PPN;
        }
        MPI_Group_incl(world_group, N, proc_list, &roots_group);
        MPI_Comm_create_from_group(roots_group, "tag", MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL,
                                   &roots_comm);
    }

    int node_sum, world_sum;
    MPI_Reduce(&world_rank, &node_sum, 1, MPI_INT, MPI_SUM, 0, node_comm);
    if (is_root) {
        MPI_Reduce(&node_sum, &world_sum, 1, MPI_INT, MPI_SUM, 0, roots_comm);
    }

    if (world_rank == 0) {
        int expect_sum = NP * (NP - 1) / 2;
        if (world_sum != expect_sum) {
            printf("Expect world_sum %d, got %d\n", expect_sum, world_sum);
        } else {
            printf("No Errors\n");
        }
    }

    MPI_Comm_free(&node_comm);
    MPI_Group_free(&node_group);
    if (is_root) {
        MPI_Comm_free(&roots_comm);
        MPI_Group_free(&roots_group);
    }
    MPI_Group_free(&world_group);

    MPI_Session_finalize(&session);

  fn_exit:
    return errs;
}
