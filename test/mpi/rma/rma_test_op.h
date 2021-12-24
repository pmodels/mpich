/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This file is included by tests such as wrma_flush_get.c to provide
 * test_op options via -op=put|acc|gacc|fop|cas.
 * Since it is expected to be included in the main test unit, it defines
 * static variables and static routines.
 */

#ifndef RMA_TEST_OP_H_INCLUDED
#define RMA_TEST_OP_H_INCLUDED

enum {
    TEST_OP_PUT,
    TEST_OP_ACC,
    TEST_OP_GACC,
    TEST_OP_FOP,
    TEST_OP_CAS,
    TEST_OP_NONE
};

static int test_op = TEST_OP_NONE;

static void parse_test_op(MTestArgList * head)
{
    const char *name = MTestArgListGetString(head, "op");
    if (strcmp(name, "put") == 0) {
        test_op = TEST_OP_PUT;
    } else if (strcmp(name, "acc") == 0) {
        test_op = TEST_OP_ACC;
    } else if (strcmp(name, "gacc") == 0) {
        test_op = TEST_OP_GACC;
    } else if (strcmp(name, "fop") == 0) {
        test_op = TEST_OP_FOP;
    } else if (strcmp(name, "cas") == 0) {
        test_op = TEST_OP_CAS;
    } else {
        test_op = TEST_OP_NONE;
    }
}

static void assert_test_op(void)
{
    if (test_op == TEST_OP_NONE) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rank == 0)
            printf("Error: must specify -op=put|acc|gacc|fop|cas\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

/* Return operation name for error message */
static const char *get_rma_name(void)
{
    switch (test_op) {
        case TEST_OP_PUT:
            return "Put";
        case TEST_OP_ACC:
            return "Accumulate";
        case TEST_OP_GACC:
            return "Get_accumulate";
        case TEST_OP_FOP:
            return "Fetch_and_op";
        case TEST_OP_CAS:
            return "Compare_and_swap";
        case TEST_OP_NONE:
            return "None";
    };
    return NULL;
}

#endif
