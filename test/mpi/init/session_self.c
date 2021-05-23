/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;

    int ret;
    MPI_Session session;
    MPI_Group group;
    MPI_Comm comm;

    ret = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &session);
    if (ret != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    ret = MPI_Group_from_session_pset(session, "mpi://self", &group);
    if (ret != MPI_SUCCESS) {
        errs++;
    }

    ret = MPI_Comm_create_from_group(group, "tag", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
    if (ret != MPI_SUCCESS) {
        errs++;
    }

    ret = MPI_Group_free(&group);
    if (ret != MPI_SUCCESS) {
        errs++;
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

    ret = MPI_Comm_free(&comm);
    if (ret != MPI_SUCCESS) {
        errs++;
    }

    ret = MPI_Session_finalize(&session);
    if (ret != MPI_SUCCESS) {
        errs++;
    }

    if (errs == 0) {
        printf("No Errors\n");
    }

  fn_exit:
    return errs;
}
