/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <string.h>

#define MTEST_LARGE_PORT_NAME
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
    int errs = 0;
    char serv_name[256] = "MyTest";
    char port_name[PORT_SIZE];
    MPI_Info port_info;
    int rank;
    MPI_Comm inter_comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    INIT_PORT_INFO(port_info);

    if (rank == 0) {
        MPI_Open_port(port_info, port_name);
        MPI_Publish_name(serv_name, port_info, port_name);
        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_SELF, &inter_comm);

        MPI_Unpublish_name(serv_name, MPI_INFO_NULL, port_name);
    } else {
        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Lookup_name(serv_name, port_info, port_name);
        MPI_Comm_connect(port_name, MPI_INFO_NULL, 0, MPI_COMM_SELF, &inter_comm);
    }

    MPI_Comm_disconnect(&inter_comm);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
