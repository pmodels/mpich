/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Test of MPI_Comm_join.  This should work even when each process
 * is started as a singleton process.
 *
 * To run
 *
 * Start the server:
 *
 * ./singjoin -s -p 54321
 *
 * In a separate shell, start the client:
 *
 * ./singjoin -c -p 54321
 *
 * where "54321" is some available port number.
 */

/* Include mpitestconf to determine if we can use h_addr or need h_addr_list */
#include "mpitestconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef HAVE_WINDOWS_H
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <windows.h>
#endif
#include <mpi.h>

#ifndef HAVE_WINDOWS_H
#define SOCKET_FD_TYPE int
#define INVALID_SOCKET_FD -1
#else
#define SOCKET_FD_TYPE  SOCKET
#define INVALID_SOCKET_FD INVALID_SOCKET
#define bcopy(s1, s2, n)    memmove(s2, s1, n)
#endif


int parse_args(int argc, char **argv);
void usage(char *name);
SOCKET_FD_TYPE server_routine(int portnum);
SOCKET_FD_TYPE client_routine(int portnum);


int is_server = 0;
int is_client = 0;
int opt_port = 0;

/* open a socket, establish a connection with a client, and return the fd */
SOCKET_FD_TYPE server_routine(int portnum)
{
    SOCKET_FD_TYPE listenfd, peer_fd;
    int ret, peer_addr_size;
    struct sockaddr_in server_addr, peer_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == INVALID_SOCKET_FD) {
        perror("server socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portnum);
    ret = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("server binding");
        return -1;
    }

    ret = listen(listenfd, 1024);
    if (ret < 0) {
        perror("server listen");
        return -1;
    }

    peer_addr_size = sizeof(peer_addr);
    peer_fd = accept(listenfd, (struct sockaddr *) &peer_addr, &peer_addr_size);

    if (peer_fd < 0) {
        perror("server accept");
        return -1;
    }
    return peer_fd;
}

/* open a socket, establish a connection with a server, and return the fd */
SOCKET_FD_TYPE client_routine(int portnum)
{
    SOCKET_FD_TYPE sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server_ent;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int hostnamelen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET_FD) {
        perror("client socket");
        return -1;
    }

    /* Try to get the processor name from MPI */
    MPI_Get_processor_name(hostname, &hostnamelen);
    server_ent = gethostbyname(hostname);
    if (server_ent == NULL) {
        fprintf(stderr, "gethostbyname fails\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portnum);
    /* POSIX might define h_addr_list only and not define h_addr */
#ifdef HAVE_H_ADDR_LIST
    bcopy(server_ent->h_addr_list[0], (char *) &server_addr.sin_addr.s_addr, server_ent->h_length);
#else
    bcopy(server_ent->h_addr, (char *) &server_addr.sin_addr.s_addr, server_ent->h_length);
#endif

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("client connect");
        return -1;
    }
    return sockfd;
}

void usage(char *name)
{
    fprintf(stderr, "usage: %s [-s|-c] -p <PORT>\n", name);
    fprintf(stderr, "      specify one and only one of -s or -c\n");
    fprintf(stderr, "      port is mandatory\n");
}

#ifdef HAVE_WINDOWS_H
static int get_option(int argc, char **argv, int arg_cnt)
{
    if (argv == NULL) {
        return -1;
    }
    if (*argv == NULL) {
        return -1;
    }
    if (arg_cnt < 0 || arg_cnt >= argc) {
        return -1;
    }
    if (strlen(argv[arg_cnt]) < 2) {
        return -1;
    }
    return (*(argv[arg_cnt] + 1));
}
#endif

int parse_args(int argc, char **argv)
{
#ifndef HAVE_WINDOWS_H
    int c;
    extern char *optarg;
    while ((c = getopt(argc, argv, "csp:")) != -1) {
        switch (c) {
        case 's':
            is_server = 1;
            break;
        case 'c':
            is_client = 1;
            break;
        case 'p':
            opt_port = atoi(optarg);
            break;
        case '?':
        case ':':
        default:
            usage(argv[0]);
            return -1;
        }
    }
    if ((is_client == 0) && (is_server == 0)) {
        usage(argv[0]);
        return -1;
    }
    if (opt_port == 0) {
        usage(argv[0]);
        return -1;
    }
    return 0;
#else
    int arg_cnt = 1;
    int c;
    while ((c = get_option(argc, argv, arg_cnt++)) > 0) {
        switch (c) {
        case 's':
            is_server = 1;
            break;
        case 'c':
            is_client = 1;
            break;
        case 'p':
            opt_port = atoi(argv[arg_cnt++]);
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }
    if ((is_client == 0) && (is_server == 0)) {
        usage(argv[0]);
        return -1;
    }
    if (opt_port == 0) {
        usage(argv[0]);
        return -1;
    }
    return 0;
#endif
}


int main(int argc, char **argv)
{
    int rank, size;
    SOCKET_FD_TYPE fd = INVALID_SOCKET_FD;
    MPI_Comm intercomm, intracomm;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != 1) {
        fprintf(stderr, "This test requires that only one process be in each comm_world\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (parse_args(argc, argv) == -1) {
        fprintf(stderr, "Unable to parse the command line arguments\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (is_server) {
        fd = server_routine(opt_port);

    }
    else if (is_client) {
        fd = client_routine(opt_port);
    }

    if (fd == INVALID_SOCKET_FD) {
        return -1;
    }

#ifdef SINGLETON_KICK
/* #warning isn't standard C, so we comment out this directive */
/* #warning  using singleton workaround */
    {
        int *usize, aflag;
        MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &usize, &aflag);
    }
#endif

    MPI_Comm_join(fd, &intercomm);

    if (is_server) {
        MPI_Send(MPI_BOTTOM, 0, MPI_INT, 0, 0, intercomm);
    }
    else {
        MPI_Recv(MPI_BOTTOM, 0, MPI_INT, 0, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Completed receive on intercomm\n");
        fflush(stdout);
    }

    MPI_Intercomm_merge(intercomm, 0, &intracomm);
    MPI_Comm_rank(intracomm, &rank);
    MPI_Comm_size(intracomm, &size);

    printf("[%d/%d] after Intercomm_merge\n", rank, size);

    MPI_Comm_free(&intracomm);
    MPI_Comm_disconnect(&intercomm);
    MPI_Finalize();
    return 0;
}
