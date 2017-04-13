/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <ws2tcpip.h>   /* socklen_t */
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif
#include <string.h>

/*
static char MTEST_Descrip[] = "A simple test of Comm_join";
*/

#define COUNT 1024

int main(int argc, char *argv[])
{
    int sendbuf[COUNT], recvbuf[COUNT], i;
    int err = 0, rank, nprocs, errs = 0;
    MPI_Comm intercomm;
    int listenfd, connfd, port, namelen;
    struct sockaddr_in cliaddr, servaddr;
    struct hostent *h;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    socklen_t len, clilen;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs != 2) {
        printf("Run this program with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 1) {
        /* server */
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd < 0) {
            printf("server cannot open socket\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = 0;

        err = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
        if (err < 0) {
            errs++;
            printf("bind failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        len = sizeof(servaddr);
        err = getsockname(listenfd, (struct sockaddr *) &servaddr, &len);
        if (err < 0) {
            errs++;
            printf("getsockname failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        port = ntohs(servaddr.sin_port);
        MPI_Get_processor_name(hostname, &namelen);

        err = listen(listenfd, 5);
        if (err < 0) {
            errs++;
            printf("listen failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Send(hostname, namelen + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&port, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);

        clilen = sizeof(cliaddr);

        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
        if (connfd < 0) {
            printf("accept failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    else {
        /* client */

        MPI_Recv(hostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 1, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&port, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Barrier(MPI_COMM_WORLD);

        h = gethostbyname(hostname);
        if (h == NULL) {
            printf("gethostbyname failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        servaddr.sin_family = h->h_addrtype;
        memcpy((char *) &servaddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
        servaddr.sin_port = htons(port);

        /* create socket */
        connfd = socket(AF_INET, SOCK_STREAM, 0);
        if (connfd < 0) {
            printf("client cannot open socket\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        /* connect to server */
        err = connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
        if (err < 0) {
            errs++;
            printf("client cannot connect\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    err = MPI_Comm_join(connfd, &intercomm);
    if (err) {
        errs++;
        printf("Error in MPI_Comm_join %d\n", err);
    }

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(intercomm, MPI_ERRORS_RETURN);


    for (i = 0; i < COUNT; i++) {
        recvbuf[i] = -1;
        sendbuf[i] = i + COUNT * rank;
    }

    err = MPI_Sendrecv(sendbuf, COUNT, MPI_INT, 0, 0, recvbuf, COUNT, MPI_INT,
                       0, 0, intercomm, MPI_STATUS_IGNORE);
    if (err != MPI_SUCCESS) {
        errs++;
        printf("Error in MPI_Sendrecv on new communicator\n");
    }

    for (i = 0; i < COUNT; i++) {
        if (recvbuf[i] != ((rank + 1) % 2) * COUNT + i)
            errs++;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    err = MPI_Comm_disconnect(&intercomm);
    if (err != MPI_SUCCESS) {
        errs++;
        printf("Error in MPI_Comm_disconnect\n");
    }

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
