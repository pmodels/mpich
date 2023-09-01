/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

void get_world_rank(MPI_Session session, int *rank)
{
    MPI_Group group = MPI_GROUP_NULL;
    MPI_Comm comm = MPI_COMM_NULL;
    int my_rank = 0;
    MPI_Group_from_session_pset(session, "mpi://WORLD", &group);
    MPI_Comm_create_from_group(group, "foo", MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &comm);
    MPI_Comm_rank(comm, &my_rank);
    MPI_Group_free(&group);
    MPI_Comm_free(&comm);
    *rank = my_rank;
}

int main(int argc, char *argv[])
{
    int errs = 0;

    int ret, world_rank, grp_size, comm_size;
    MPI_Session session = MPI_SESSION_NULL;
    MPI_Group group = MPI_GROUP_NULL;
    MPI_Comm comm = MPI_COMM_NULL;
#ifdef WITH_WORLD
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
#endif

    ret = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &session);
    if (ret != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    /* Get the world rank of the process */
    get_world_rank(session, &world_rank);

    ret = MPI_Group_from_session_pset(session, "mpi://SELF", &group);
    if (ret != MPI_SUCCESS) {
        errs++;
    }

    MPI_Group_size(group, &grp_size);
    if (grp_size != 1) {
        printf("Error: Size of group generated from pset mpi://SELF is %d (expected size 1)\n",
               grp_size);
        errs++;
        goto fn_exit;
    }

    ret = MPI_Comm_create_from_group(group, "tag", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
    if (ret != MPI_SUCCESS) {
        errs++;
    }

    if (comm == MPI_COMM_NULL) {
        printf("Error: communicator for pset mpi://SELF is MPI_COMM_NULL\n");
        errs++;
        goto fn_exit;
    }

    MPI_Comm_size(comm, &comm_size);
    if (comm_size != grp_size) {
        printf("Error: Comm for pset mpi://SELF has size %d (expected size 1)\n", comm_size);
        errs++;
        goto fn_exit;
    }

    MPI_Request req;
    int send_data, recv_data;
    int rank = 0;
    int tag = 0;

    ret = MPI_Irecv(&recv_data, 1, MPI_INT, rank, tag, comm, &req);
    if (ret != MPI_SUCCESS) {
        errs++;
    }
    ret = MPI_Send(&send_data, 1, MPI_INT, rank, tag, comm);
    if (ret != MPI_SUCCESS) {
        errs++;
    }
    ret = MPI_Wait(&req, MPI_STATUS_IGNORE);
    if (ret != MPI_SUCCESS) {
        errs++;
    }

  fn_exit:
    if (group != MPI_GROUP_NULL) {
        ret = MPI_Group_free(&group);
        if (ret != MPI_SUCCESS) {
            errs++;
        }
    }

    if (comm != MPI_COMM_NULL) {
        ret = MPI_Comm_free(&comm);
        if (ret != MPI_SUCCESS) {
            errs++;
        }
    }

    if (session != MPI_SESSION_NULL) {
        ret = MPI_Session_finalize(&session);
        if (ret != MPI_SUCCESS) {
            errs++;
        }
    }

    if (world_rank == 0) {
        if (errs == 0) {
            printf("No Errors\n");
        } else {
            printf("%d Errors\n", errs);
        }
    }
#ifdef WITH_WORLD
    MPI_Finalize();
#endif
    return errs;
}
