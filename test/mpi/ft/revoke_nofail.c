/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail_test(int rank, const char *test_name, const char *operation, int rc)
{
    int errclass = MPI_SUCCESS;
    int resultlen = 0;
    char error[MPI_MAX_ERROR_STRING] = "";

    MPI_Error_class(rc, &errclass);
    MPI_Error_string(rc, error, &resultlen);
    fprintf(stderr,
            "[%d] %s: %s should have returned MPIX_ERR_REVOKED (%d), but rc=%d class=%d (%s)\n",
            rank, test_name, operation, MPIX_ERR_REVOKED, rc, errclass, error);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

static void require_revoked(int rank, const char *test_name, const char *operation, int rc)
{
    int errclass = MPI_SUCCESS;

    MPI_Error_class(rc, &errclass);
    if (errclass != MPIX_ERR_REVOKED) {
        fail_test(rank, test_name, operation, rc);
    }
}

static MPI_Comm dup_world_with_errhandler(void)
{
    MPI_Comm comm;

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);
    return comm;
}

static void sync_world(void)
{
    MPI_Barrier(MPI_COMM_WORLD);
}

static void test_barrier_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;

    if (rank == 1) {
        MPIX_Comm_revoke(comm);
    }

    rc = MPI_Barrier(comm);
    require_revoked(rank, "barrier_after_revoke", "MPI_Barrier", rc);
    MPI_Comm_free(&comm);
}

static void test_send_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    int sendbuf = rank;
    int recvbuf = -1;

    if (rank == 1) {
        MPIX_Comm_revoke(comm);
    }
    sync_world();

    if (rank == 0) {
        rc = MPI_Send(&sendbuf, 1, MPI_INT, 1, 100, comm);
        require_revoked(rank, "send_after_revoke", "MPI_Send", rc);
    } else if (rank == 1) {
        rc = MPI_Recv(&recvbuf, 1, MPI_INT, 0, 100, comm, MPI_STATUS_IGNORE);
        require_revoked(rank, "send_after_revoke", "MPI_Recv", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_isend_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    int sendbuf = rank;
    int recvbuf = -1;
    MPI_Request req = MPI_REQUEST_NULL;

    if (rank == 0) {
        rc = MPI_Irecv(&recvbuf, 1, MPI_INT, 0, 101, comm, &req);
        if (rc != MPI_SUCCESS) {
            fail_test(rank, "isend_after_revoke", "MPI_Irecv setup", rc);
        }
    }

    sync_world();
    if (rank == 1) MPIX_Comm_revoke(comm);

    if (rank == 1) {
        rc = MPI_Isend(&sendbuf, 1, MPI_INT, 1, 101, comm, &req);
        require_revoked(rank, "isend_after_revoke", "MPI_Isend", rc);
    } else if (rank == 0) {
        rc = MPI_Wait(&req, MPI_STATUS_IGNORE);
        require_revoked(rank, "isend_after_revoke", "MPI_Irecv", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_sendrecv_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    int sendbuf = rank;
    int recvbuf = -1;
    int peer = (rank + 1) % 2;

    if (rank == 1) {
        MPIX_Comm_revoke(comm);
    }
    sync_world();

    if (rank < 2) {
        rc = MPI_Sendrecv(&sendbuf, 1, MPI_INT, peer, 102,
                          &recvbuf, 1, MPI_INT, peer, 102, comm, MPI_STATUS_IGNORE);
        require_revoked(rank, "sendrecv_after_revoke", "MPI_Sendrecv", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_probe_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    MPI_Status status;

    if (rank == 1) {
        MPIX_Comm_revoke(comm);
    }
    sync_world();

    if (rank == 0) {
        rc = MPI_Probe(1, 103, comm, &status);
        require_revoked(rank, "probe_after_revoke", "MPI_Probe", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_irecv_unmatched_before_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    int recvbuf = -1;
    MPI_Request req = MPI_REQUEST_NULL;

    if (rank == 0) {
        rc = MPI_Irecv(&recvbuf, 1, MPI_INT, 1, 104, comm, &req);
        if (rc != MPI_SUCCESS) {
            fail_test(rank, "irecv_unmatched_before_revoke", "MPI_Irecv setup", rc);
        }
    }

    sync_world();
    if (rank == 1) {
        MPIX_Comm_revoke(comm);
    }

    if (rank == 0) {
        rc = MPI_Wait(&req, MPI_STATUS_IGNORE);
        require_revoked(rank, "irecv_unmatched_before_revoke", "MPI_Wait(Irecv)", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_irecv_matched_only_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    int sendbuf = rank;
    int recvbuf = -1;
    MPI_Request req = MPI_REQUEST_NULL;

    if (rank == 0) {
        rc = MPI_Irecv(&recvbuf, 1, MPI_INT, 1, 105, comm, &req);
        if (rc != MPI_SUCCESS) {
            fail_test(rank, "irecv_matched_only_after_revoke", "MPI_Irecv setup", rc);
        }
    }

    sync_world();
    if (rank == 1) MPIX_Comm_revoke(comm);

    if (rank == 1) {
        rc = MPI_Send(&sendbuf, 1, MPI_INT, 0, 105, comm);
        require_revoked(rank, "irecv_matched_only_after_revoke", "MPI_Send after revoke", rc);
    } else if (rank == 0) {
        rc = MPI_Wait(&req, MPI_STATUS_IGNORE);
        require_revoked(rank, "irecv_matched_only_after_revoke", "MPI_Wait(Irecv)", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_ibarrier_unmatched_until_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    MPI_Request req = MPI_REQUEST_NULL;

    if (rank != 1) {
        rc = MPI_Ibarrier(comm, &req);
        if (rc != MPI_SUCCESS) {
            fail_test(rank, "ibarrier_unmatched_until_after_revoke", "MPI_Ibarrier setup", rc);
        }
    }

    sync_world();
    if (rank == 1) MPIX_Comm_revoke(comm);

    if (rank == 1) {
        rc = MPI_Ibarrier(comm, &req);
        require_revoked(rank, "ibarrier_unmatched_until_after_revoke", "MPI_Ibarrier after revoke", rc);
    } else if (rank == 0) {
        MPI_Status status;
        rc = MPI_Wait(&req, &status);
        if (rc == MPI_SUCCESS) {
            require_revoked(rank, "ibarrier_unmatched_until_after_revoke", "MPI_Wait(Ibarrier) status.MPI_ERROR", status.MPI_ERROR);
        }
        require_revoked(rank, "ibarrier_unmatched_until_after_revoke", "MPI_Wait(Ibarrier)", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_ibarrier_unmatched_forever_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    MPI_Request req = MPI_REQUEST_NULL;

    if (rank != 1) {
        rc = MPI_Ibarrier(comm, &req);
        if (rc != MPI_SUCCESS) {
            fail_test(rank, "ibarrier_unmatched_forever_after_revoke", "MPI_Ibarrier setup", rc);
        }
    }

    sync_world();
    if (rank == 1) MPIX_Comm_revoke(comm);

    if (rank != 1) {
        rc = MPI_Wait(&req, MPI_STATUS_IGNORE);
        require_revoked(rank, "ibarrier_unmatched_forever_after_revoke", "MPI_Wait(Ibarrier)", rc);
    }

    MPI_Comm_free(&comm);
}

static void test_ibarrier_after_revoke(int rank)
{
    MPI_Comm comm = dup_world_with_errhandler();
    int rc;
    MPI_Request req = MPI_REQUEST_NULL;

    sync_world();
    if (rank == 1) MPIX_Comm_revoke(comm);

    rc = MPI_Ibarrier(comm, &req);
    if (rc != MPI_SUCCESS) {
        require_revoked(rank, "ibarrier_after_revoke", "MPI_Ibarrier", rc);
    } else {
        rc = MPI_Wait(&req, MPI_STATUS_IGNORE);
        require_revoked(rank, "ibarrier_after_revoke", "MPI_Wait(Ibarrier)", rc);
    }

    MPI_Comm_free(&comm);
}

int main(int argc, char **argv)
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        fprintf(stderr, "Must run with at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    test_barrier_after_revoke(rank);
    sync_world();
    test_send_after_revoke(rank);
    sync_world();
    test_isend_after_revoke(rank);
    sync_world();
    test_sendrecv_after_revoke(rank);
    sync_world();
    test_probe_after_revoke(rank);
    sync_world();
    test_irecv_unmatched_before_revoke(rank);
    sync_world();
    test_irecv_matched_only_after_revoke(rank);
    sync_world();
    test_ibarrier_unmatched_until_after_revoke(rank);
    sync_world();
    test_ibarrier_unmatched_forever_after_revoke(rank);
    sync_world();
    test_ibarrier_after_revoke(rank);
    sync_world();

    if (rank == 0) {
        fprintf(stdout, " No errors\n");
    }

    MPI_Finalize();

    return 0;
}
