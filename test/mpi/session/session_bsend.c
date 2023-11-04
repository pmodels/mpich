/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

int errs = 0;

static int test_bsend(MPI_Comm comm);

int main(int argc, char *argv[])
{
    MPI_Session session;
    MPI_Group group;
    MPI_Comm comm;

    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &session);

    MPI_Group_from_session_pset(session, "mpi://world", &group);
    MPI_Comm_create_from_group(group, "session_bsend", MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &comm);

#define BUFSIZE 1024    /* assume it is sufficient to cover MPI_BSEND_OVERHEAD */
    char buffer[BUFSIZE];
    char *dummy_buf;
    int dummy_size;

    MPI_Buffer_attach(buffer, BUFSIZE);
    errs += test_bsend(comm);
    MPI_Buffer_detach(&dummy_buf, &dummy_size);
    if (dummy_buf != buffer)
        printf("MPI_Buffer_detach: dummy_buf = %p, expect %p\n", dummy_buf, buffer);
    if (dummy_size != BUFSIZE)
        printf("MPI_Buffer_detach: dummy_size = %d, expect %d\n", dummy_size, BUFSIZE);

#if MTEST_HAVE_MIN_MPI_VERSION(4,1)
    MPI_Session_attach_buffer(session, buffer, BUFSIZE);
    errs += test_bsend(comm);
    MPI_Session_detach_buffer(session, &dummy_buf, &dummy_size);
    if (dummy_buf != buffer)
        printf("MPI_Session_detach_buffer: dummy_buf = %p, expect %p\n", dummy_buf, buffer);
    if (dummy_size != BUFSIZE)
        printf("MPI_Session_detach_buffer: dummy_size = %d, expect %d\n", dummy_size, BUFSIZE);

    MPI_Comm_attach_buffer(comm, buffer, BUFSIZE);
    errs += test_bsend(comm);
    MPI_Comm_detach_buffer(comm, &dummy_buf, &dummy_size);
    if (dummy_buf != buffer)
        printf("MPI_Comm_detach_buffer: dummy_buf = %p, expect %p\n", dummy_buf, buffer);
    if (dummy_size != BUFSIZE)
        printf("MPI_Comm_detach_buffer: dummy_size = %d, expect %d\n", dummy_size, BUFSIZE);

    /* automatic buffer */

    MPI_Buffer_attach(MPI_BUFFER_AUTOMATIC, 0);
    errs += test_bsend(comm);
    MPI_Buffer_detach(&dummy_buf, &dummy_size);
    if (dummy_buf != MPI_BUFFER_AUTOMATIC)
        printf("MPI_Buffer_detach: dummy_buf = %p, expect %p\n", dummy_buf, MPI_BUFFER_AUTOMATIC);
    if (dummy_size != 0)
        printf("MPI_Buffer_detach: dummy_size = %d, expect %d\n", dummy_size, 0);

    MPI_Session_attach_buffer(session, MPI_BUFFER_AUTOMATIC, 0);
    errs += test_bsend(comm);
    MPI_Session_detach_buffer(session, &dummy_buf, &dummy_size);
    if (dummy_buf != MPI_BUFFER_AUTOMATIC)
        printf("MPI_Session_detach_buffer: dummy_buf = %p, expect %p\n", dummy_buf,
               MPI_BUFFER_AUTOMATIC);
    if (dummy_size != 0)
        printf("MPI_Session_detach_buffer: dummy_size = %d, expect %d\n", dummy_size, 0);

    MPI_Comm_attach_buffer(comm, MPI_BUFFER_AUTOMATIC, 0);
    errs += test_bsend(comm);
    MPI_Comm_detach_buffer(comm, &dummy_buf, &dummy_size);
    if (dummy_buf != MPI_BUFFER_AUTOMATIC)
        printf("MPI_Comm_detach_buffer: dummy_buf = %p, expect %p\n", dummy_buf,
               MPI_BUFFER_AUTOMATIC);
    if (dummy_size != 0)
        printf("MPI_Comm_detach_buffer: dummy_size = %d, expect %d\n", dummy_size, 0);

    /* large count variations */

    MPI_Count dummy_size_c;
    MPI_Buffer_attach_c(MPI_BUFFER_AUTOMATIC, 0);
    errs += test_bsend(comm);
    MPI_Buffer_detach_c(&dummy_buf, &dummy_size_c);
    if (dummy_buf != MPI_BUFFER_AUTOMATIC)
        printf("MPI_Buffer_detach_c: dummy_buf = %p, expect %p\n", dummy_buf, MPI_BUFFER_AUTOMATIC);
    if (dummy_size != 0)
        printf("MPI_Buffer_detach_c: dummy_size = %d, expect %d\n", (int) dummy_size, 0);

    MPI_Session_attach_buffer_c(session, MPI_BUFFER_AUTOMATIC, 0);
    errs += test_bsend(comm);
    MPI_Session_detach_buffer_c(session, &dummy_buf, &dummy_size_c);
    if (dummy_buf != MPI_BUFFER_AUTOMATIC)
        printf("MPI_Session_detach_buffer_c: dummy_buf = %p, expect %p\n", dummy_buf,
               MPI_BUFFER_AUTOMATIC);
    if (dummy_size != 0)
        printf("MPI_Session_detach_buffer_c: dummy_size = %d, expect %d\n", (int) dummy_size, 0);

    MPI_Comm_attach_buffer_c(comm, MPI_BUFFER_AUTOMATIC, 0);
    errs += test_bsend(comm);
    MPI_Comm_detach_buffer_c(comm, &dummy_buf, &dummy_size_c);
    if (dummy_buf != MPI_BUFFER_AUTOMATIC)
        printf("MPI_Comm_detach_buffer_c: dummy_buf = %p, expect %p\n", dummy_buf,
               MPI_BUFFER_AUTOMATIC);
    if (dummy_size != 0)
        printf("MPI_Comm_detach_buffer_c: dummy_size = %d, expect %d\n", (int) dummy_size, 0);
#endif

    int rank;
    MPI_Comm_rank(comm, &rank);
    if (rank == 0 && errs == 0) {
        printf("No Errors\n");
    }
    MPI_Comm_free(&comm);
    MPI_Group_free(&group);
    MPI_Session_finalize(&session);
    return MTestReturnValue(errs);
}

static int test_bsend(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int src = 0;
    int dst = size > 1 ? size - 1 : 0;
    int tag = 1234;

    int err = 0;
#define N 10
    int buf[N];
    if (rank == src) {
        for (int i = 0; i < N; i++) {
            buf[i] = i;
        }
        MPI_Bsend(buf, N, MPI_INT, dst, tag, comm);

        /* get err from dst */
        int err;
        MPI_Recv(&err, 1, MPI_INT, dst, tag, comm, MPI_STATUS_IGNORE);
    }
    if (rank == dst) {
        for (int i = 0; i < N; i++) {
            buf[i] = -1;
        }
        MPI_Recv(buf, N, MPI_INT, src, tag, comm, MPI_STATUS_IGNORE);

        /* check err */
        for (int i = 0; i < N; i++) {
            if (buf[i] != i) {
                err++;
            }
        }
        MPI_Send(&err, 1, MPI_INT, src, tag, comm);
    }
    return err;
}
