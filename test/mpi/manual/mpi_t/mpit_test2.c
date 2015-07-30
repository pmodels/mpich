/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* A simple test of the proposed MPI_T_ interface that assumes the
 * presence of two performance variables: "posted_recvq_length" and
 * "unexpected_recvq_length".  Some information about these variables
 * is printed to stdout and then a few messages are sent/received to
 * show that theses variables are reporting useful information.
 *
 * Author: Dave Goodell <goodell@mcs.anl.gov.
 */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "mpitestconf.h"

#if !defined(USE_STRICT_MPI) && defined(MPICH)

static int posted_qlen, unexpected_qlen;
static MPI_Aint posted_queue_match_attempts, unexpected_queue_match_attempts;
static MPI_T_pvar_handle pq_handle, uq_handle, pqm_handle, uqm_handle;
static MPI_T_pvar_session session;

static void print_vars(int idx)
{
    MPI_T_pvar_read(session, pq_handle, &posted_qlen);
    MPI_T_pvar_read(session, uq_handle, &unexpected_qlen);
    MPI_T_pvar_read(session, pqm_handle, &posted_queue_match_attempts);
    MPI_T_pvar_read(session, uqm_handle, &unexpected_queue_match_attempts);

    printf("(%d) posted_qlen=%d unexpected_qlen=%d ", idx, posted_qlen, unexpected_qlen);
    printf("posted_queue_match_attempts=%lu unexpected_queue_match_attempts=%lu\n",
           posted_queue_match_attempts, unexpected_queue_match_attempts);
}

int main(int argc, char **argv)
{
    int i;
    int num;
    int rank, size;
/*#define STR_SZ (15)*/
#define STR_SZ (50)
    int name_len = STR_SZ;
    char name[STR_SZ] = "";
    int desc_len = STR_SZ;
    char desc[STR_SZ] = "";
    int verb;
    MPI_Datatype dtype;
    int count;
    int bind;
    int varclass;
    int readonly, continuous, atomic;
    int provided;
    MPI_T_enum enumtype;
    int pq_idx = -1, uq_idx = -1, pqm_idx = -1, uqm_idx = -1;
    int pqm_writable = -1, uqm_writable = -1;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    provided = 0xdeadbeef;
    MPI_T_init_thread(MPI_THREAD_SINGLE, &provided);
    assert(provided != 0xdeadbeef);

    num = 0xdeadbeef;
    MPI_T_pvar_get_num(&num);
    printf("get_num=%d\n", num);
    assert(num != 0xdeadbeef);
    for (i = 0; i < num; ++i) {
        name_len = desc_len = STR_SZ;
        MPI_T_pvar_get_info(i, name, &name_len, &verb, &varclass, &dtype, &enumtype, desc,
                            &desc_len, &bind, &readonly, &continuous, &atomic);
        printf("index=%d\n", i);
        printf("--> name='%s' name_len=%d desc='%s' desc_len=%d\n", name, name_len, desc, desc_len);
        printf("--> verb=%d varclass=%d dtype=%#x bind=%d readonly=%d continuous=%d atomic=%d\n",
               verb, varclass, dtype, bind, readonly, continuous, atomic);

        if (0 == strcmp(name, "posted_recvq_length")) {
            pq_idx = i;
        }
        else if (0 == strcmp(name, "unexpected_recvq_length")) {
            uq_idx = i;
        }
        else if (0 == strcmp(name, "posted_recvq_match_attempts")) {
            pqm_idx = i;
            pqm_writable = !readonly;
        }
        else if (0 == strcmp(name, "unexpected_recvq_match_attempts")) {
            uqm_idx = i;
            uqm_writable = !readonly;
        }
    }

    printf("pq_idx=%d uq_idx=%d pqm_idx=%d uqm_idx=%d\n", pq_idx, uq_idx, pqm_idx, uqm_idx);

    /* setup a session and handles for the PQ and UQ length variables */
    session = MPI_T_PVAR_SESSION_NULL;
    MPI_T_pvar_session_create(&session);
    assert(session != MPI_T_PVAR_SESSION_NULL);

    pq_handle = MPI_T_PVAR_HANDLE_NULL;
    MPI_T_pvar_handle_alloc(session, pq_idx, NULL, &pq_handle, &count);
    assert(count = 1);
    assert(pq_handle != MPI_T_PVAR_HANDLE_NULL);

    uq_handle = MPI_T_PVAR_HANDLE_NULL;
    MPI_T_pvar_handle_alloc(session, uq_idx, NULL, &uq_handle, &count);
    assert(count = 1);
    assert(uq_handle != MPI_T_PVAR_HANDLE_NULL);

    pqm_handle = MPI_T_PVAR_HANDLE_NULL;
    MPI_T_pvar_handle_alloc(session, pqm_idx, NULL, &pqm_handle, &count);
    assert(count = 1);
    assert(pqm_handle != MPI_T_PVAR_HANDLE_NULL);

    uqm_handle = MPI_T_PVAR_HANDLE_NULL;
    MPI_T_pvar_handle_alloc(session, uqm_idx, NULL, &uqm_handle, &count);
    assert(count = 1);
    assert(uqm_handle != MPI_T_PVAR_HANDLE_NULL);

    /* now send/recv some messages and track the lengths of the queues */
    {
        int buf1, buf2, buf3, buf4;
        MPI_Request r1, r2, r3, r4;

        buf1 = buf2 = buf3 = buf4 = 0xfeedface;
        r1 = r2 = r3 = r4 = MPI_REQUEST_NULL;

        posted_qlen = 0x0123abcd;
        unexpected_qlen = 0x0123abcd;
        posted_queue_match_attempts = 0x0123abcd;
        unexpected_queue_match_attempts = 0x0123abcd;
        print_vars(1);

        MPI_Isend(&buf1, 1, MPI_INT, 0, /*tag= */ 11, MPI_COMM_SELF, &r1);
        print_vars(2);
        printf("expected (posted_qlen,unexpected_qlen) = (0,1)\n");

        MPI_Isend(&buf1, 1, MPI_INT, 0, /*tag= */ 22, MPI_COMM_SELF, &r2);
        print_vars(3);
        printf("expected (posted_qlen,unexpected_qlen) = (0,2)\n");

        MPI_Irecv(&buf2, 1, MPI_INT, 0, /*tag= */ 33, MPI_COMM_SELF, &r3);
        print_vars(4);
        printf("expected (posted_qlen,unexpected_qlen) = (1,2)\n");

        MPI_Recv(&buf3, 1, MPI_INT, 0, /*tag= */ 22, MPI_COMM_SELF, MPI_STATUS_IGNORE);
        MPI_Wait(&r2, MPI_STATUS_IGNORE);
        print_vars(5);
        printf("expected (posted_qlen,unexpected_qlen) = (1,1)\n");

        MPI_Recv(&buf3, 1, MPI_INT, 0, /*tag= */ 11, MPI_COMM_SELF, MPI_STATUS_IGNORE);
        MPI_Wait(&r1, MPI_STATUS_IGNORE);
        print_vars(6);
        printf("expected (posted_qlen,unexpected_qlen) = (1,0)\n");

        MPI_Send(&buf3, 1, MPI_INT, 0, /*tag= */ 33, MPI_COMM_SELF);
        MPI_Wait(&r3, MPI_STATUS_IGNORE);
        print_vars(7);
        printf("expected (posted_qlen,unexpected_qlen) = (0,0)\n");
    }

    if (pqm_writable) {
        posted_queue_match_attempts = 0;
        MPI_T_pvar_write(session, pqm_handle, &posted_queue_match_attempts);
    }
    if (uqm_writable) {
        unexpected_queue_match_attempts = 0;
        MPI_T_pvar_write(session, uqm_handle, &unexpected_queue_match_attempts);
    }
    print_vars(8);

    /* cleanup */
    MPI_T_pvar_handle_free(session, &uqm_handle);
    MPI_T_pvar_handle_free(session, &pqm_handle);
    MPI_T_pvar_handle_free(session, &uq_handle);
    MPI_T_pvar_handle_free(session, &pq_handle);
    MPI_T_pvar_session_free(&session);

    MPI_T_finalize();
    MPI_Finalize();

    return 0;
}
#else
/* Simple null program to allow building this file with non-MPICH
   implementations */
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    printf(" No Errors\n");
    MPI_Finalize();
    return 0;
}
#endif
