/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test checks that the nemesis code correctly exposes statistics related
 * to unexpected receive queue buffer/message sizes.
 *
 * Originally written by Ralf Gunter Correa Carvalho. */

#include <mpi.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define TRY(func)                           \
    do {                                    \
        err = (func);                       \
        if (err != MPI_SUCCESS)             \
            MPI_Abort(MPI_COMM_WORLD, err); \
    } while (0)

#define EAGER_SIZE 10
#define RNDV_SIZE  100000
#define STR_LEN    100

int err, rank;
MPI_T_pvar_session session;
MPI_T_pvar_handle uqsize_handle;

/* The first receive will block waiting for the last send, since messages from
 * a given rank are received in order. */
void reversed_tags_test()
{
    size_t unexpected_recvq_buffer_size;

    if (rank == 0) {
        int send_buf[EAGER_SIZE] = { 0x1234 };

        MPI_Send(send_buf, EAGER_SIZE, MPI_INT, 1, 0xA, MPI_COMM_WORLD);
        MPI_Send(send_buf, EAGER_SIZE, MPI_INT, 1, 0xB, MPI_COMM_WORLD);
        MPI_Send(send_buf, EAGER_SIZE, MPI_INT, 1, 0xC, MPI_COMM_WORLD);
        MPI_Send(send_buf, EAGER_SIZE, MPI_INT, 1, 0xD, MPI_COMM_WORLD);
    }
    else if (rank == 1) {
        int recv_buf[EAGER_SIZE];
        MPI_Status status;

        MPI_Recv(recv_buf, EAGER_SIZE, MPI_INT, 0, 0xD, MPI_COMM_WORLD, &status);
        TRY(MPI_T_pvar_read(session, uqsize_handle, &unexpected_recvq_buffer_size));
        assert(unexpected_recvq_buffer_size == 3 * EAGER_SIZE * sizeof(int));

        MPI_Recv(recv_buf, EAGER_SIZE, MPI_INT, 0, 0xC, MPI_COMM_WORLD, &status);
        TRY(MPI_T_pvar_read(session, uqsize_handle, &unexpected_recvq_buffer_size));
        assert(unexpected_recvq_buffer_size == 2 * EAGER_SIZE * sizeof(int));

        MPI_Recv(recv_buf, EAGER_SIZE, MPI_INT, 0, 0xB, MPI_COMM_WORLD, &status);
        TRY(MPI_T_pvar_read(session, uqsize_handle, &unexpected_recvq_buffer_size));
        assert(unexpected_recvq_buffer_size == 1 * EAGER_SIZE * sizeof(int));

        MPI_Recv(recv_buf, EAGER_SIZE, MPI_INT, 0, 0xA, MPI_COMM_WORLD, &status);
        TRY(MPI_T_pvar_read(session, uqsize_handle, &unexpected_recvq_buffer_size));
        assert(unexpected_recvq_buffer_size == 0 * EAGER_SIZE * sizeof(int));
    }

    MPI_Barrier(MPI_COMM_WORLD);        /* make sure this test is over before going to the next one */
}

/* Rendezvous-based messages will never be unexpected (except for the initial RTS,
 * which has an empty buffer anyhow).
 */
void rndv_test()
{
    size_t unexpected_recvq_buffer_size;

    if (rank == 0) {
        int send_buf[RNDV_SIZE] = { 0x5678 };

        MPI_Send(send_buf, RNDV_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(send_buf, RNDV_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    else if (rank == 1) {
        int recv_buf[RNDV_SIZE];
        MPI_Status status;

        MPI_Recv(recv_buf, RNDV_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        TRY(MPI_T_pvar_read(session, uqsize_handle, &unexpected_recvq_buffer_size));
        assert(unexpected_recvq_buffer_size == 0);

        MPI_Recv(recv_buf, RNDV_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        TRY(MPI_T_pvar_read(session, uqsize_handle, &unexpected_recvq_buffer_size));
        assert(unexpected_recvq_buffer_size == 0);
    }

    MPI_Barrier(MPI_COMM_WORLD);        /* make sure this test is over before going to the next one */
}

int main(int argc, char *argv[])
{
    int i, size, num, name_len, desc_len, verb, thread_support;
    int varclass, bind, readonly, continuous, atomic, uqsize_idx, count;
    char name[STR_LEN], desc[STR_LEN];
    MPI_Datatype dtype;
    MPI_T_enum enumtype;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("MPIT pvar test: unexpected_recvq_buffer_size\n");
        fflush(stdout);
    }

    /* Ensure we're using exactly two ranks. */
    /* Future tests (using collectives) might need this because of the MPI_Barrier */
    assert(size == 2);

    /* Standard MPIT initialization. */
    TRY(MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_support));
    TRY(MPI_T_pvar_get_num(&num));

    int found = 0;

    /* Locate desired MPIT variable. */
    for (i = 0; i < num; i++) {
        name_len = desc_len = STR_LEN;
        TRY(MPI_T_pvar_get_info(i, name, &name_len, &verb, &varclass, &dtype,
                                &enumtype, desc, &desc_len, &bind, &readonly,
                                &continuous, &atomic));

        if (strcmp(name, "unexpected_recvq_buffer_size") == 0) {
            uqsize_idx = i;
            found = 1;
        }
    }

    if (found) {
        /* Initialize MPIT session & variable handle. */
        MPI_T_pvar_session_create(&session);
        MPI_T_pvar_handle_alloc(session, uqsize_idx, NULL, &uqsize_handle, &count);

        /* Ensure the variable is of the correct size. */
        assert(count == 1);

        /* Run a batch of tests. */
        reversed_tags_test();
        rndv_test();

        /* Cleanup. */
        MPI_T_pvar_handle_free(session, &uqsize_handle);
        MPI_T_pvar_session_free(&session);
    }

    if (rank == 0) {
        printf("finished\n");
        fflush(stdout);
    }

    TRY(MPI_T_finalize());
    MPI_Finalize();

    return 0;
}
