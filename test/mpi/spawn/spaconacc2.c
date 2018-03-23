/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"

/* Note: In this program, the return codes from the MPI routines are checked.
   Since the error handlers for the communicators are not set to
   MPI_ERRORS_RETURN, any error should cause an abort rather than a return.
   The test on the return value is an extra safety check; note that a
   return value of other than MPI_SUCCESS in these routines indicates an
   error in the error handling by the MPI implementation */

#define IF_VERBOSE(a) if (verbose) { printf a ; fflush(stdout); }

void check_error(int error, const char *fcname);
void check_error(int error, const char *fcname)
{
    char err_string[MPI_MAX_ERROR_STRING] = "";
    int length;
    if (error != MPI_SUCCESS) {
        MPI_Error_string(error, err_string, &length);
        printf("%s failed: %s\n", fcname, err_string);
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, error);
    }
}

int main(int argc, char *argv[])
{
    int error;
    int rank, size, i;
    char *argv1[2] = { (char *) "connector", NULL };
    char *argv2[2] = { (char *) "acceptor", NULL };
    MPI_Comm comm_connector, comm_acceptor, comm_parent, comm;
    char port[MPI_MAX_PORT_NAME];
    MPI_Status status;
    MPI_Info spawn_path = MPI_INFO_NULL;
    int verbose = 0;
    int can_spawn, errs = 0;

    if (getenv("MPITEST_VERBOSE")) {
        verbose = 1;
    }

    IF_VERBOSE(("init.\n"));
    error = MPI_Init(&argc, &argv);
    check_error(error, "MPI_Init");

    errs += MTestSpawnPossible(&can_spawn);
    if (!can_spawn) {
        if (errs)
            printf(" Found %d errors\n", errs);
        else
            printf(" No Errors\n");
        fflush(stdout);
    } else {
        IF_VERBOSE(("size.\n"));
        error = MPI_Comm_size(MPI_COMM_WORLD, &size);
        check_error(error, "MPI_Comm_size");

        IF_VERBOSE(("rank.\n"));
        error = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        check_error(error, "MPI_Comm_rank");

        if (argc == 1) {
            /* Make sure that the current directory is in the path.
             * Not all implementations may honor or understand this, but
             * it is highly recommended as it gives users a clean way
             * to specify the location of the executable without
             * specifying a particular directory format (e.g., this
             * should work with both Windows and Unix implementations) */
            MPI_Info_create(&spawn_path);
            MPI_Info_set(spawn_path, (char *) "path", (char *) ".");

            IF_VERBOSE(("spawn connector.\n"));
            error = MPI_Comm_spawn((char *) "spaconacc2", argv1, 1, spawn_path, 0,
                                   MPI_COMM_SELF, &comm_connector, MPI_ERRCODES_IGNORE);
            check_error(error, "MPI_Comm_spawn");

            IF_VERBOSE(("spawn acceptor.\n"));
            error = MPI_Comm_spawn((char *) "spaconacc2", argv2, 1, spawn_path, 0,
                                   MPI_COMM_SELF, &comm_acceptor, MPI_ERRCODES_IGNORE);
            check_error(error, "MPI_Comm_spawn");
            MPI_Info_free(&spawn_path);

            IF_VERBOSE(("recv port.\n"));
            error = MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_acceptor, &status);
            check_error(error, "MPI_Recv");

            IF_VERBOSE(("send port.\n"));
            error = MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_connector);
            check_error(error, "MPI_Send");

            IF_VERBOSE(("barrier acceptor.\n"));
            error = MPI_Barrier(comm_acceptor);
            check_error(error, "MPI_Barrier");

            IF_VERBOSE(("barrier connector.\n"));
            error = MPI_Barrier(comm_connector);
            check_error(error, "MPI_Barrier");

            error = MPI_Comm_free(&comm_acceptor);
            check_error(error, "MPI_Comm_free");
            error = MPI_Comm_free(&comm_connector);
            check_error(error, "MPI_Comm_free");

            printf(" No Errors\n");
        } else if ((argc == 2) && (strcmp(argv[1], "acceptor") == 0)) {
            IF_VERBOSE(("get_parent.\n"));
            error = MPI_Comm_get_parent(&comm_parent);
            check_error(error, "MPI_Comm_get_parent");
            if (comm_parent == MPI_COMM_NULL) {
                printf("acceptor's parent is NULL.\n");
                fflush(stdout);
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            IF_VERBOSE(("open_port.\n"));
            error = MPI_Open_port(MPI_INFO_NULL, port);
            check_error(error, "MPI_Open_port");

            IF_VERBOSE(("0: opened port: <%s>\n", port));
            IF_VERBOSE(("send.\n"));
            error = MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_parent);
            check_error(error, "MPI_Send");

            IF_VERBOSE(("accept.\n"));
            error = MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
            check_error(error, "MPI_Comm_accept");

            IF_VERBOSE(("close_port.\n"));
            error = MPI_Close_port(port);
            check_error(error, "MPI_Close_port");

            /* Send some data before disconnecting */
            IF_VERBOSE(("sending int.\n"));
            i = 123;
            error = MPI_Send(&i, 1, MPI_INT, 0, 0, comm);
            check_error(error, "MPI_Send");

            IF_VERBOSE(("disconnect.\n"));
            error = MPI_Comm_disconnect(&comm);
            check_error(error, "MPI_Comm_disconnect");

            IF_VERBOSE(("barrier.\n"));
            error = MPI_Barrier(comm_parent);
            check_error(error, "MPI_Barrier");

            MPI_Comm_free(&comm_parent);
        } else if ((argc == 2) && (strcmp(argv[1], "connector") == 0)) {
            IF_VERBOSE(("get_parent.\n"));
            error = MPI_Comm_get_parent(&comm_parent);
            check_error(error, "MPI_Comm_get_parent");
            if (comm_parent == MPI_COMM_NULL) {
                printf("acceptor's parent is NULL.\n");
                fflush(stdout);
                MPI_Abort(MPI_COMM_WORLD, -1);
            }

            IF_VERBOSE(("recv.\n"));
            error = MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_parent, &status);
            check_error(error, "MPI_Recv");

            IF_VERBOSE(("1: received port: <%s>\n", port));
            IF_VERBOSE(("connect.\n"));
            error = MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
            check_error(error, "MPI_Comm_connect");

            /* Receive some data before disconnecting */
            IF_VERBOSE(("receiving int\n"));
            i = -1;
            error = MPI_Recv(&i, 1, MPI_INT, 0, 0, comm, &status);
            check_error(error, "MPI_Recv");
            if (i != 123) {
                printf("expected 123 but received %d\n", i);
                fflush(stdout);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            IF_VERBOSE(("disconnect.\n"));
            error = MPI_Comm_disconnect(&comm);
            check_error(error, "MPI_Comm_disconnect");

            IF_VERBOSE(("barrier.\n"));
            error = MPI_Barrier(comm_parent);
            check_error(error, "MPI_Barrier");

            MPI_Comm_free(&comm_parent);
        } else {
            printf("invalid command line.\n");
            fflush(stdout);
            {
                int ii;
                for (ii = 0; ii < argc; ii++) {
                    printf("argv[%d] = <%s>\n", ii, argv[ii]);
                }
            }
            fflush(stdout);
            MPI_Abort(MPI_COMM_WORLD, -2);
        }
    }

    MPI_Finalize();
    return MTestReturnValue(errs);
}
