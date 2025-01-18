/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"

#define IF_VERBOSE(a) if (verbose) { printf a ; fflush(stdout); }

/* This test checks to make sure that two MPI_Comm_connects to two different
 * MPI ports
 * match their corresponding MPI_Comm_accepts.  The root process opens two
 * MPI ports and
 * sends the first port to process 1 and the second to process 2.  Then the
 * root process
 * accepts a connection from the second port followed by the first port.  '
 * Processes 1 and
 * 2 both connect back to the root but process 2 first sleeps for three
 * seconds to give
 * process 1 time to attempt to connect to the root.  The root should wait
 * until
 * process 2 connects before accepting the connection from process 1.
 */

#ifdef MTEST_LARGE_PORT_NAME
#define PORT_SIZE 4096

#define INIT_PORT_INFO(info) \
    do { \
        MPI_Info_create(&(info)); \
        MPI_Info_set(info, "port_name_size", "4096"); \
    } while (0)

#define FREE_PORT_INFO(info) MPI_Info_free(&(info))

#else
#define PORT_SIZE MPI_MAX_PORT_NAME
#define INIT_PORT_INFO(info) do {info = MPI_INFO_NULL;} while (0)
#define FREE_PORT_INFO(info) do { } while (0)

#endif /* MTEST_LARGE_PORT_NAME */

int main(int argc, char *argv[])
{
    int num_errors = 0, total_num_errors = 0;
    int rank, size;
    char port1[PORT_SIZE];
    char port2[PORT_SIZE];
    char port3[PORT_SIZE];
    MPI_Info port_info;
    MPI_Status status;
    MPI_Comm comm1, comm2, comm3;
    int verbose = 0;
    int data = 0;

    MTEST_VG_MEM_INIT(port1, PORT_SIZE * sizeof(char));
    MTEST_VG_MEM_INIT(port2, PORT_SIZE * sizeof(char));
    MTEST_VG_MEM_INIT(port3, PORT_SIZE * sizeof(char));

    if (getenv("MPITEST_VERBOSE")) {
        verbose = 1;
    }

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 4) {
        printf("Four processes needed to run this test.\n");
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        IF_VERBOSE(("0: opening ports.\n"));
        INIT_PORT_INFO(port_info);
        MPI_Open_port(port_info, port1);
        MPI_Open_port(port_info, port2);
        MPI_Open_port(port_info, port3);
        FREE_PORT_INFO(port_info);

        IF_VERBOSE(("0: opened port1: <%s>\n", port1));
        IF_VERBOSE(("0: opened port2: <%s>\n", port2));
        IF_VERBOSE(("0: opened port3: <%s>\n", port3));
        IF_VERBOSE(("0: sending ports.\n"));
        MPI_Send(port1, PORT_SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Send(port2, PORT_SIZE, MPI_CHAR, 2, 0, MPI_COMM_WORLD);
        MPI_Send(port3, PORT_SIZE, MPI_CHAR, 3, 0, MPI_COMM_WORLD);

        IF_VERBOSE(("0: accepting port3.\n"));
        MPI_Comm_accept(port3, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm3);
        IF_VERBOSE(("0: accepting port2.\n"));
        MPI_Comm_accept(port2, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm2);
        IF_VERBOSE(("0: accepting port1.\n"));
        MPI_Comm_accept(port1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm1);

        IF_VERBOSE(("0: closing ports.\n"));
        MPI_Close_port(port1);
        MPI_Close_port(port2);
        MPI_Close_port(port3);

        IF_VERBOSE(("0: sending 1 to process 1.\n"));
        data = 1;
        MPI_Send(&data, 1, MPI_INT, 0, 0, comm1);

        IF_VERBOSE(("0: sending 2 to process 2.\n"));
        data = 2;
        MPI_Send(&data, 1, MPI_INT, 0, 0, comm2);

        IF_VERBOSE(("0: sending 3 to process 3.\n"));
        data = 3;
        MPI_Send(&data, 1, MPI_INT, 0, 0, comm3);

        IF_VERBOSE(("0: disconnecting.\n"));
        MPI_Comm_disconnect(&comm1);
        MPI_Comm_disconnect(&comm2);
        MPI_Comm_disconnect(&comm3);
    } else if (rank == 1) {
        IF_VERBOSE(("1: receiving port.\n"));
        MPI_Recv(port1, PORT_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);

        IF_VERBOSE(("1: received port1: <%s>\n", port1));
        IF_VERBOSE(("1: connecting.\n"));
        MPI_Comm_connect(port1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm1);

        MPI_Recv(&data, 1, MPI_INT, 0, 0, comm1, &status);
        if (data != 1) {
            printf("Received %d from root when expecting 1\n", data);
            fflush(stdout);
            num_errors++;
        }

        IF_VERBOSE(("1: disconnecting.\n"));
        MPI_Comm_disconnect(&comm1);
    } else if (rank == 2) {
        IF_VERBOSE(("2: receiving port.\n"));
        MPI_Recv(port2, PORT_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);

        IF_VERBOSE(("2: received port2: <%s>\n", port2));
        /* make sure process 1 has time to do the connect before this process
         * attempts to connect */
        MTestSleep(2);
        IF_VERBOSE(("2: connecting.\n"));
        MPI_Comm_connect(port2, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm2);

        MPI_Recv(&data, 1, MPI_INT, 0, 0, comm2, &status);
        if (data != 2) {
            printf("Received %d from root when expecting 2\n", data);
            fflush(stdout);
            num_errors++;
        }

        IF_VERBOSE(("2: disconnecting.\n"));
        MPI_Comm_disconnect(&comm2);
    } else if (rank == 3) {
        IF_VERBOSE(("3: receiving port.\n"));
        MPI_Recv(port3, PORT_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);

        IF_VERBOSE(("2: received port2: <%s>\n", port2));
        /* make sure process 1 and 2 have time to do the connect before this
         * process attempts to connect */
        MTestSleep(4);
        IF_VERBOSE(("3: connecting.\n"));
        MPI_Comm_connect(port3, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm3);

        MPI_Recv(&data, 1, MPI_INT, 0, 0, comm3, &status);
        if (data != 3) {
            printf("Received %d from root when expecting 3\n", data);
            fflush(stdout);
            num_errors++;
        }

        IF_VERBOSE(("3: disconnecting.\n"));
        MPI_Comm_disconnect(&comm3);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Finalize(num_errors);
    return MTestReturnValue(total_num_errors);
}
