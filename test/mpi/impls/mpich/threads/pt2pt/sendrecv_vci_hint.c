/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"
#include "mpithreadtest.h"
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define NTHREADS 4
#define ITERATIONS 100

/*
  This program aims at testing the comm hints that are likely to affect
  vci_idx selection for pt2pt operations. They can be hints from MPI
  standard or non-standard hints. It provides testcases to evaluate
  functional correctness of MPI when running pt2pt operations after
  applying such comm hints. This is a multi-threaded test.
 */

struct thread_param {
    int id;                     /* Thread id */
    int iter;
    MPI_Comm comm;
    int tag;
    int result;
    bool maintain_msg_order;    /* Useful for the hint allow overtaking */
};

/* A simple send recv test that is used by the test threads */
MTEST_THREAD_RETURN_TYPE run_send_recv_test(void *arg)
{
    struct thread_param *p = (struct thread_param *) arg;
    int buff[p->iter];
    MPI_Status status[p->iter];
    int i, rank, errs = 0;
    int thread_id_mult = 10000;

    MTest_thread_barrier(NTHREADS);
    MPI_Comm_rank(p->comm, &rank);
    for (i = 0; i < p->iter; i++) {
        if (rank == 0) {
            buff[i] = (thread_id_mult * p->id);
            /* If the hint mpi_assert_allow_overtaking is present, msg from
             * the different iterations can match in any order. So, we keep
             * the data value same across the iterations. That way the data
             * validation in this test after the recv will not pick it as
             error */
            if (p->maintain_msg_order)
                buff[i] += i;
            MPI_Send(&buff[i], 1, MPI_INT, 1, p->tag, p->comm);
        } else {
            buff[i] = -1;
            MPI_Recv(&buff[i], 1, MPI_INT, 0, p->tag, p->comm, &status[i]);
            /* Data validation */
            if ((p->maintain_msg_order && buff[i] != i + thread_id_mult * p->id)
                || !p->maintain_msg_order && buff[i] != thread_id_mult * p->id) {
                errs++;
            }
        }
    }
    MTest_thread_barrier(NTHREADS);
    p->result = errs;
    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

/* Launch multiple threads, apply provided comm hints, run send/recv */
static int comm_hint_test(const char **comm_hints, const char **comm_hints_vals, int n_hints,
                          bool test_set_value, bool maintain_msg_order)
{
    int i, j, errs = 0, nprocs, rank, flag;
    struct thread_param params[NTHREADS];
    MPI_Comm dup_world;
    MPI_Info comm_info;
    char hint_buf[MPI_MAX_INFO_VAL];

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Record the provided comm hints and create comm dup */
    if (n_hints == 0) {
        MPI_Comm_dup_with_info(MPI_COMM_WORLD, MPI_INFO_NULL, &dup_world);
    } else {
        /* Create info object and pass it for comm dup creation */
        MPI_Info_create(&comm_info);
        for (j = 0; j < n_hints; j++) {
            MPI_Info_set(comm_info, comm_hints[j], comm_hints_vals[j]);
        }
        MPI_Comm_dup_with_info(MPI_COMM_WORLD, comm_info, &dup_world);

        /* Read back the set hint value */
        if (test_set_value) {
            MPI_Info comm_info_out;
            MPI_Comm_get_info(dup_world, &comm_info_out);
            for (j = 0; j < n_hints; j++) {
                memset(hint_buf, 0, MPI_MAX_INFO_VAL * sizeof(char));
                MPI_Info_get(comm_info_out, comm_hints[j], MPI_MAX_INFO_VAL, hint_buf, &flag);
                if (!flag) {
                    fprintf(stderr, "Error while reading hint : %s\n", comm_hints[j]);
                    errs++;
                }

                if (strcmp(hint_buf, comm_hints_vals[j])) {
                    fprintf(stderr,
                            "Error while reading value of hint : %s, value read: %s instead of: %s\n",
                            comm_hints[j], hint_buf, comm_hints_vals[j]);
                    errs++;
                }
            }
            MPI_Info_free(&comm_info_out);
        }
    }

    /* Setup and launch the test threads */
    for (i = 0; i < NTHREADS; i++) {
        params[i].comm = dup_world;
        params[i].tag = i;      /* Set tag = thread id */
        params[i].iter = ITERATIONS;
        params[i].id = i;
        params[i].maintain_msg_order = maintain_msg_order;
        if (i == NTHREADS - 1)
            run_send_recv_test(&params[i]);
        else
            MTest_Start_thread(run_send_recv_test, &params[i]);
    }
    MTest_Join_threads();

    /* Cleanup comm objects */
    if (n_hints > 0)
        MPI_Info_free(&comm_info);
    MPI_Comm_free(&dup_world);

    for (i = 0; i < NTHREADS; i++)
        errs += params[i].result;

    return errs;
}

int main(int argc, char **argv)
{
    int pmode, nprocs, errs = 0, err;
    const char *hints_1[2];
    const char *hints_1_vals[2];

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (nprocs < 2) {
        fprintf(stdout, "Need two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MTest_thread_barrier_init();
    if (err) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test : no hint
     * Perform send/recv with no comm hint applied. Just for
     * sanity check.
     */
    errs += comm_hint_test(NULL, NULL, 0, false, false);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Standard comm hints tests.
     * The following standard comm hints are expected to guide how
     * vci_idx are chosen for both sender and receiver. So, this test
     * aims to trigger the vci_idx selection. After applying this hint,
     * we expect send-recv to run without any crash or hangup.
     */

    /* Test : hints assert_no_any_tag */
    hints_1[0] = "mpi_assert_no_any_tag";
    hints_1_vals[0] = "true";
    errs += comm_hint_test(hints_1, hints_1_vals, 1, true, true);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Test : hints assert_no_any_tag and assert_no_any_source */
    hints_1[0] = "mpi_assert_no_any_source";
    hints_1_vals[0] = "true";
    errs += comm_hint_test(hints_1, hints_1_vals, 1, true, true);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Test : hints assert_no_any_tag and assert_no_any_source */
    hints_1[0] = "mpi_assert_no_any_tag";
    hints_1[1] = "mpi_assert_no_any_source";
    hints_1_vals[0] = "true";
    hints_1_vals[1] = "true";
    errs += comm_hint_test(hints_1, hints_1_vals, 2, true, true);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Test : hint assert_allow_overtaking */
    hints_1[0] = "mpi_assert_allow_overtaking";
    hints_1_vals[0] = "true";
    errs += comm_hint_test(hints_1, hints_1_vals, 1, true, false);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Non-standard comm hints tests.
     * The following non-standard hints allow user to directly
     * suggest what vci_idx to use for sender and receiver. Again, with
     * these hints, we expect send-recv to run successfully.
     */

    /* Test : hints sender_vci and receiver_vci with valid values.
     * Here we expect the program to run successfully when these valid
     * values are provided.
     */
    hints_1[0] = "sender_vci";
    hints_1[1] = "receiver_vci";
    hints_1_vals[0] = "2";
    hints_1_vals[1] = "3";
    errs += comm_hint_test(hints_1, hints_1_vals, 2, false, true);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Test : sender_vci and receiver_vci with potential invalid values
     * Here we try to pass invalid values for sender and reveiver vci_id.
     * We expect MPI to still use a valid vci_idx, e.g., by doing mod on
     * the provided hint. So, send-recv should still run successfully.
     */
    hints_1[0] = "sender_vci";
    hints_1[1] = "receiver_vci";
    hints_1_vals[0] = "99999";
    hints_1_vals[1] = "99999";
    errs += comm_hint_test(hints_1, hints_1_vals, 2, false, true);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Test : vci hint.
     * Here we expect the program to run successfully when valid
     * vci values are provided.
     */
    hints_1[0] = "vci";
    hints_1_vals[0] = "2";
    errs += comm_hint_test(hints_1, hints_1_vals, 1, false, true);
    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Finalize(errs);
    return errs;
}
