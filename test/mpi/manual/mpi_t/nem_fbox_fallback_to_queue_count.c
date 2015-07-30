/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test checks that the nemesis code correctly exposes statistics related
 * to "fbox" handling.  It also attempts to verify that it accurately maintains
 * these statistics.
 *
 * Originally written by Ralf Gunter Correa Carvalho. */

#include <mpi.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "mpitest.h"

#define TRY(func)                           \
    do {                                    \
        err = (func);                       \
        if (err != MPI_SUCCESS)             \
            MPI_Abort(MPI_COMM_WORLD, err); \
    } while (0)

#define STR_LEN   100
#define BUF_COUNT 10

uint64_t null_fbox[2] = { 0 };

int err, rank;
MPI_T_pvar_session session;
MPI_T_pvar_handle fbox_handle;

/* Check that we can successfuly write to the variable.
 * Question: Do we really want to write pvars other than reset?
 */
void blank_test()
{
    uint64_t temp[2] = { -1 };

    temp[0] = 0x1234;
    temp[1] = 0xABCD;
    TRY(MPI_T_pvar_write(session, fbox_handle, temp));

    temp[0] = 0xCD34;
    temp[1] = 0x12AB;
    TRY(MPI_T_pvar_read(session, fbox_handle, temp));
    assert(temp[0] == 0x1234);
    assert(temp[1] == 0xABCD);
}

/* Nemesis' fastbox falls back to regular queues when more than one message
 * is yet to be delivered.
 * Here, the sender posts all sends before the receiver has a chance to
 * acknowledge any of them; this should force the sender to fall_back to the
 * queue every time. */
void send_first_test()
{
    uint64_t nem_fbox_fall_back_to_queue_count[2] = { -1 };

    /* Reset the fbox variable. */
    MPI_T_pvar_reset(session, fbox_handle);

    if (rank == 0) {
        char send_buf[BUF_COUNT] = { 0x12 };

        /* Check that the variable has been correctly initialized. */
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 0);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 0);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 1);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 2);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 3);

        /* Make sure we've posted the sends before the receiver gets a chance
         * to receive them.
         * FIXME: Ideally this should use a barrier, but that uses messages
         *        internally and hence will sometimes screw up the asserts above.
         */
        MTestSleep(1);

    }
    else if (rank == 1) {
        char recv_buf[BUF_COUNT];
        MPI_Status status;

        MTestSleep(1);  /* see above */

        MPI_Recv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    }

    MPI_Barrier(MPI_COMM_WORLD);        /* ensure we've finished this test before
                                         * moving on to the next */
}

/* By posting receives ahead of time, messages should be taken out of the
 * fastbox as soon as they are delivered.  Hence, the counter should remain 0
 * throughout.
 * FIXME: This doesn't quite work yet, in part because of the barrier (which
 *        also uses messages).  May want to 'sleep' between sends as a
 *        workaround.
 */
void recv_first_test()
{
    uint64_t nem_fbox_fall_back_to_queue_count[2] = { -1 };

    /* Reset the fbox variable. */
    MPI_T_pvar_reset(session, fbox_handle);

    if (rank == 0) {
        char send_buf[BUF_COUNT] = { 0x12 };

        MPI_Barrier(MPI_COMM_WORLD);    /* see below */

        /* Check that the variable has been correctly initialized. */
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 0);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 0);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 0);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 0);

        MPI_Send(send_buf, BUF_COUNT, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        TRY(MPI_T_pvar_read(session, fbox_handle, nem_fbox_fall_back_to_queue_count));
        assert(nem_fbox_fall_back_to_queue_count[1] == 0);
    }
    else if (rank == 1) {
        char recv_buf[BUF_COUNT];
        MPI_Request reqs[4];

        MPI_Irecv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &reqs[0]);
        MPI_Irecv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &reqs[1]);
        MPI_Irecv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &reqs[2]);
        MPI_Irecv(recv_buf, BUF_COUNT, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &reqs[3]);

        MPI_Barrier(MPI_COMM_WORLD);    /* make sure we've posted the receives
                                         * before the sender gets a chance
                                         * to send them */

        MPI_Status status[4];
        MPI_Waitall(4, reqs, status);
    }

    MPI_Barrier(MPI_COMM_WORLD);        /* ensure we've finished this test before
                                         * moving on to the next */
}

int main(int argc, char *argv[])
{
    int i, size, num, name_len, desc_len, count, verb, thread_support;
    int varclass, bind, readonly, continuous, atomic;
    int fbox_idx = -1;
    char name[STR_LEN], desc[STR_LEN];
    MPI_Datatype dtype;
    MPI_T_enum enumtype;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("MPIT pvar test: nem_fbox_fall_back_to_queue_count\n");
        fflush(stdout);
    }

    /* Ensure we're using exactly two ranks. */
    assert(size == 2);

    /* Standard MPIT initialization. */
    TRY(MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_support));
    TRY(MPI_T_pvar_get_num(&num));

    /* Locate desired MPIT variable. */
    for (i = 0; i < num; i++) {
        name_len = desc_len = STR_LEN;
        TRY(MPI_T_pvar_get_info(i, name, &name_len, &verb, &varclass, &dtype,
                                &enumtype, desc, &desc_len, &bind, &readonly,
                                &continuous, &atomic));

        if (strcmp(name, "nem_fbox_fall_back_to_queue_count") == 0)
            fbox_idx = i;
    }

    /* Ensure variable was registered by the runtime */
    assert(fbox_idx != -1);

    /* Initialize MPIT session & variable handle. */
    MPI_T_pvar_session_create(&session);
    MPI_T_pvar_handle_alloc(session, fbox_idx, NULL, &fbox_handle, &count);

    /* Ensure the variable is of the correct size. */
    assert(count == 2);

    /* Run a batch of tests. */
    /* blank_test(); */
    send_first_test();
    /* recv_first_test(); */

    /* Cleanup. */
    MPI_T_pvar_handle_free(session, &fbox_handle);
    MPI_T_pvar_session_free(&session);

    if (rank == 0) {
        printf("finished\n");
        fflush(stdout);
    }

    TRY(MPI_T_finalize());
    MPI_Finalize();

    return 0;
}
