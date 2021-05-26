/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* A simple test of the proposed MPI_T_ interface that assumes the
 * presence of two events: "unexp_message_enqueued" and
 * "unexp_message_dequeued".  Some information about these events
 * is printed to stdout and then a few messages are sent/received to
 * show that the call backs for these events are triggered.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitestconf.h"
#include "mpitest.h"

static int errs = 0;

static void uenq_cb_fn(MPI_T_event_instance event_instance, MPI_T_event_registration er,
                       MPI_T_cb_safety cb_safety, void *user_data)
{
    int rank;
    int tag;
    MPI_T_event_read(event_instance, 0, &rank);
    MPI_T_event_read(event_instance, 1, &tag);
    if (rank != 0 || tag != 11)
        errs = 1;
}

static void udeq_cb_fn(MPI_T_event_instance event_instance, MPI_T_event_registration er,
                       MPI_T_cb_safety cb_safety, void *user_data)
{
    int rank;
    int tag;
    MPI_T_event_read(event_instance, 0, &rank);
    MPI_T_event_read(event_instance, 1, &tag);
    if (rank != 0 || tag != 11)
        errs = 1;
}

static void recvq_free_cb(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety,
                          void *user_data)
{
    return;
}

int main(int argc, char *argv[])
{
    int i;
    int num;
    int rank, size;
    char *name;
    char *desc;
    int verbos;
    MPI_Datatype *dt;
    MPI_Aint *at;
    int num_elements;
    int bind;
    int provided, maxnamelen, maxdesclen, maxne = 0;
    MPI_T_enum enumtype;
    MPI_Info info;
    int uenq_idx = -1, udeq_idx = -1;
    MPI_T_event_registration uenq_er, udeq_er;

    MTest_Init(&argc, &argv);
    MPI_T_init_thread(MPI_THREAD_SINGLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_T_event_get_num(&num);
    if (num == 0) {
        MPI_T_finalize();
        MTest_Finalize(errs);
        return MTestReturnValue(errs);
    }

    for (i = 0; i < num; ++i) {
        int namelen = 0;
        int desclen = 0;
        char fname[5];
        char fdesc[5];
        MPI_T_event_get_info(i, fname, &namelen, &verbos, NULL, NULL, &num_elements, &enumtype,
                             &info, fdesc, &desclen, &bind);
        if (namelen > maxnamelen)
            maxnamelen = namelen;
        if (desclen > maxdesclen)
            maxdesclen = desclen;
        if (num_elements > maxne)
            maxne = num_elements;
    }

    /* Allocate string buffers */
    name = (char *) malloc(sizeof(char) * maxnamelen);
    desc = (char *) malloc(sizeof(char) * maxdesclen);

    dt = (MPI_Datatype *) malloc(sizeof(MPI_Datatype) * maxne);
    at = (MPI_Aint *) malloc(sizeof(MPI_Aint) * maxne);

    for (i = 0; i < num; ++i) {
        MPI_T_event_get_info(i, name, &maxnamelen, &verbos, dt, at, &num_elements, &enumtype, &info,
                             desc, &maxdesclen, &bind);
        if (0 == strcmp(name, "unexp_message_enqueued")) {
            uenq_idx = i;
        } else if (0 == strcmp(name, "unexp_message_dequeued")) {
            udeq_idx = i;
        }
    }

    /* setup handles for the ENQ and DEQ events */
    MPI_T_event_handle_alloc(uenq_idx, NULL, info, &uenq_er);
    MPI_T_event_handle_alloc(udeq_idx, NULL, info, &udeq_er);

    /* register callbacks for the events */
    MPI_T_event_register_callback(uenq_er, MPI_T_CB_REQUIRE_MPI_RESTRICTED, info, NULL,
                                  (MPI_T_event_cb_function *) uenq_cb_fn);
    MPI_T_event_register_callback(udeq_er, MPI_T_CB_REQUIRE_MPI_RESTRICTED, info, NULL,
                                  (MPI_T_event_cb_function *) udeq_cb_fn);

    /* now send/recv some messages and track the lengths of the queues */
    {
        int buf1, buf2, buf3, buf4;
        MPI_Request r1, r2, r3, r4;

        buf1 = buf2 = buf3 = buf4 = 0xfeedface;
        r1 = r2 = r3 = r4 = MPI_REQUEST_NULL;


        MPI_Isend(&buf1, 1, MPI_INT, 0, /*tag= */ 11, MPI_COMM_SELF, &r1);

        MPI_Isend(&buf1, 1, MPI_INT, 0, /*tag= */ 22, MPI_COMM_SELF, &r2);

        MPI_Irecv(&buf2, 1, MPI_INT, 0, /*tag= */ 33, MPI_COMM_SELF, &r3);

        MPI_Recv(&buf3, 1, MPI_INT, 0, /*tag= */ 22, MPI_COMM_SELF, MPI_STATUS_IGNORE);
        MPI_Wait(&r2, MPI_STATUS_IGNORE);

        MPI_Recv(&buf3, 1, MPI_INT, 0, /*tag= */ 11, MPI_COMM_SELF, MPI_STATUS_IGNORE);
        MPI_Wait(&r1, MPI_STATUS_IGNORE);

        MPI_Send(&buf3, 1, MPI_INT, 0, /*tag= */ 33, MPI_COMM_SELF);
        MPI_Wait(&r3, MPI_STATUS_IGNORE);
    }

    /* cleanup */
    free(name);
    free(desc);
    free(dt);
    free(at);
    MPI_T_event_handle_free(uenq_er, NULL, (MPI_T_event_free_cb_function *) recvq_free_cb);
    MPI_T_event_handle_free(udeq_er, NULL, (MPI_T_event_free_cb_function *) recvq_free_cb);

    MPI_T_finalize();
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
