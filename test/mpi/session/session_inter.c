/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <assert.h>

/* This test tests MPI_Intercomm_create_from_groups */

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
    if (world_size == 1) {
        printf("The test require more than 1 process.\n");
        goto fn_exit;
    }

    int *proc_list;
    int local_leader, remote_leader;
    proc_list = malloc(world_size * sizeof(int));
    for (int i = 0; i < world_size; i++) {
        proc_list[i] = i;
    }

    int n1 = world_size / 2;
    int n2 = world_size - n1;
    MPI_Group local_group, remote_group;
    if (world_rank < n1) {
        MPI_Group_incl(world_group, n1, proc_list, &local_group);
        MPI_Group_incl(world_group, n2, proc_list + n1, &remote_group);
    } else {
        MPI_Group_incl(world_group, n1, proc_list, &remote_group);
        MPI_Group_incl(world_group, n2, proc_list + n1, &local_group);
    }

    free(proc_list);

    MPI_Comm inter_comm;
    MPI_Intercomm_create_from_groups(local_group, 0, remote_group, 0, "string tag",
                                     MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &inter_comm);

    errs = MTestTestIntercomm(inter_comm);

    MPI_Group_free(&world_group);
    MPI_Group_free(&local_group);
    MPI_Group_free(&remote_group);
    MPI_Comm_free(&inter_comm);

    MPI_Session_finalize(&session);

    if (world_rank == 0 && errs == 0) {
        printf("No Errors\n");
    }

  fn_exit:
    return errs;
}
