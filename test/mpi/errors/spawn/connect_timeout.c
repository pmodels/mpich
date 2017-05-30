/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"

#define IF_VERBOSE(cond, a) if (verbose && (cond)) { printf a ; fflush(stdout); }
#define SERVER_GID 0
#define CLIENT_GID 1
#define TIMEOUT "5"

/* This test checks whether a MPI_COMM_CONNECT can eventually time out and return
 * an error of class MPI_ERR_PORT when the named port exists but no pending
 * MPI_COMM_ACCEPT on server side. We divide all processes into a server group
 * and a client group, the client group issues connect to the server group.
 *
 * Two test cases are performed in separately:
 * (1) No_accept: Server only calls open/close_port while client tries to connect.
 *     Each connect call should return MPI_ERR_PORT.
 * (2) Mismatched: Server only issues one accept while client tries twice connect.
 *     At least one of the connect call should return MPI_ERR_PORT. */

int rank, nproc;
int verbose = 0;

int test_mismatched_accept(MPI_Comm intra_comm, int gid);
int test_no_accept(MPI_Comm intra_comm, int gid);

#if defined(TEST_MISMATCH)
#define RUN_TEST(comm, gid) test_mismatched_accept(comm, gid)
#else /* no accept by default */
#define RUN_TEST(comm, gid) test_no_accept(comm, gid)
#endif

/* Check whether error clase is equal to the expected one. */
static inline int check_errno(int mpi_errno, int expected_errno)
{
    int errclass = MPI_SUCCESS;
    int err = 0;

    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != expected_errno) {
        char errstring[MPI_MAX_ERROR_STRING];
        char exp_errstring[MPI_MAX_ERROR_STRING];
        int errstrlen = 0;

        err++;
        MPI_Error_string(mpi_errno, errstring, &errstrlen);
        MPI_Error_string(expected_errno, exp_errstring, &errstrlen);
        fprintf(stderr, "Error: returned wrong error class %s, expected %s\n",
                errstring, exp_errstring);
    }

    return err;
}

/* The first process in server group opens port and broadcast to all other
 * processes in the world.  */
static inline void open_and_bcast_port(int intra_rank, int gid, char (*port)[])
{
    int server_rank = 0, local_server_rank = 0;

    /* server root opens port */
    if (intra_rank == 0 && gid == SERVER_GID) {
        local_server_rank = rank;

        MPI_Open_port(MPI_INFO_NULL, (*port));
        IF_VERBOSE(1, ("server root: opened port1: <%s>\n", (*port)));
    }

    /* broadcast world rank of server root, then broadcast port */
    MPI_Allreduce(&local_server_rank, &server_rank, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Bcast((*port), MPI_MAX_PORT_NAME, MPI_CHAR, server_rank, MPI_COMM_WORLD);
}

int test_mismatched_accept(MPI_Comm intra_comm, int gid)
{
    char port[MPI_MAX_PORT_NAME];
    MPI_Comm comm = MPI_COMM_NULL;
    int mpi_errno = MPI_SUCCESS;
    int errs = 0;
    int intra_rank, intra_nproc;
    int server_rank = 0, local_server_rank = 0;

    MTEST_VG_MEM_INIT(port, MPI_MAX_PORT_NAME * sizeof(char));

    MPI_Comm_rank(intra_comm, &intra_rank);
    MPI_Comm_size(intra_comm, &intra_nproc);
    IF_VERBOSE(intra_rank == 0, ("%s: TEST mismatched accept case, %d nprocs in group\n",
                                 (gid == SERVER_GID) ? "server" : "client", intra_nproc));

    open_and_bcast_port(intra_rank, gid, &port);

    /* client group */
    if (gid == CLIENT_GID) {
        int mpi_errno1 = MPI_SUCCESS, mpi_errno2 = MPI_SUCCESS;

        IF_VERBOSE(intra_rank == 0, ("client: connecting to <%s> with default timeout.\n", port));
        mpi_errno1 = MPI_Comm_connect(port, MPI_INFO_NULL, 0, intra_comm, &comm);
        if (mpi_errno1 != MPI_SUCCESS) {
            errs += check_errno(mpi_errno1, MPI_ERR_PORT);
        }

        /* At least one of the connect calls should return MPI_ERR_PORT */
        IF_VERBOSE(intra_rank == 0, ("client: connecting to <%s> again.\n", port));
        mpi_errno2 = MPI_Comm_connect(port, MPI_INFO_NULL, 0, intra_comm, &comm);
        if (mpi_errno1 == MPI_SUCCESS) {
            errs += check_errno(mpi_errno2, MPI_ERR_PORT);
        } else {
            errs += check_errno(mpi_errno2, MPI_SUCCESS);
        }
    } else if (gid == SERVER_GID) {
        /* NOTE: if accept hangs, try increase MPIR_CVAR_CH3_COMM_CONN_TIMEOUT. */
        IF_VERBOSE(intra_rank == 0, ("server: accepting connection to <%s>.\n", port));
        MPI_Comm_accept(port, MPI_INFO_NULL, 0, intra_comm, &comm);
    }

    if (comm != MPI_COMM_NULL) {
        IF_VERBOSE(intra_rank == 0, ("connection matched, freeing communicator.\n"));
        MPI_Comm_free(&comm);
    }

    if (intra_rank == 0 && gid == SERVER_GID) {
        IF_VERBOSE(1, ("server root: closing port.\n"));
        MPI_Close_port(port);
    }

    return errs;
}

int test_no_accept(MPI_Comm intra_comm, int gid)
{
    char port[MPI_MAX_PORT_NAME], timeout = 0;
    MPI_Info info = MPI_INFO_NULL;
    MPI_Comm comm = MPI_COMM_NULL;
    int mpi_errno = MPI_SUCCESS;
    int errs = 0;
    int intra_rank, intra_nproc;
    int server_rank = 0, local_server_rank = 0;

    MTEST_VG_MEM_INIT(port, MPI_MAX_PORT_NAME * sizeof(char));

    MPI_Comm_rank(intra_comm, &intra_rank);
    MPI_Comm_size(intra_comm, &intra_nproc);
    IF_VERBOSE(intra_rank == 0, ("%s: TEST no accept case, %d nprocs in group\n",
                                 (gid == SERVER_GID) ? "server" : "client", intra_nproc));

    open_and_bcast_port(intra_rank, gid, &port);

    /* client group */
    if (gid == CLIENT_GID) {
        IF_VERBOSE(intra_rank == 0,
                   ("client: case 1 - connecting to <%s> with default timeout.\n", port));
        mpi_errno = MPI_Comm_connect(port, MPI_INFO_NULL, 0, intra_comm, &comm);
        errs += check_errno(mpi_errno, MPI_ERR_PORT);

        MPI_Info_create(&info);
        MPI_Info_set(info, "timeout", TIMEOUT);

        IF_VERBOSE(intra_rank == 0,
                   ("client: case 2 - connecting to <%s> with specified timeout <%s>s.\n", port,
                    TIMEOUT));
        mpi_errno = MPI_Comm_connect(port, info, 0, intra_comm, &comm);
        errs += check_errno(mpi_errno, MPI_ERR_PORT);

        MPI_Info_free(&info);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* server closes port after client finished connection */
    if (intra_rank == 0 && gid == SERVER_GID) {
        IF_VERBOSE(1, ("server root: closing port.\n"));
        MPI_Close_port(port);
    }

    return errs;
}


int main(int argc, char *argv[])
{
    MPI_Comm intra_comm = MPI_COMM_NULL;
    int sub_rank, sub_nproc;
    int errs = 0, allerrs = 0;

    if (getenv("MPITEST_VERBOSE")) {
        verbose = 1;
    }

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nproc < 2) {
        printf("At least two processes needed to run this test.\n");
        goto exit;
    }

    /* Separate all processes into a server group and a client group.
     * Processes in client group connect to server group. */
    MPI_Comm_split(MPI_COMM_WORLD, rank % 2, rank, &intra_comm);
    MPI_Errhandler_set(intra_comm, MPI_ERRORS_RETURN);

    errs = RUN_TEST(intra_comm, rank % 2);

    MPI_Comm_free(&intra_comm);

  exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
