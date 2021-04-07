/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"
#include <stdbool.h>

/*
  This program provides implementation of tests cases for the functional
  evaluation of the support for MPI communicator info hints.
 */

/* Read given info hint key value from the given info object,
 * while expecting the hint value=val */
static int ReadCommInfo(MPI_Info info, const char *key, const char *val, char *buf,
                        const char *test_name)
{
    int flag;
    int errors = 0;

    memset(buf, 0, MPI_MAX_INFO_VAL * sizeof(char));
    MPI_Info_get(info, key, MPI_MAX_INFO_VAL, buf, &flag);

    if (!flag) {
        fprintf(stderr, "Testing %s: hint %s was not set\n", test_name, key);
        errors++;
    }
    if (strcmp(buf, val)) {
        fprintf(stderr, "Testing %s: Expected value: %s, received: %s\n", test_name, val, buf);
        errors++;
    }
    return errors;
}

int main(int argc, char **argv)
{
    int i, rank;
    int errors = 0, errs = 0;
    char query_key[MPI_MAX_INFO_KEY];
    char buf[MPI_MAX_INFO_VAL];
    char val[MPI_MAX_INFO_VAL];
    MPI_Comm comm_dup1;
    /* comm_dup2 not used */
    MPI_Comm comm_dup3;
    MPI_Comm comm_dup4;
    MPI_Comm comm_split5;
    MPI_Info info_in1;
    MPI_Info info_in4;
    MPI_Info info_out1;
    /* info_out2 not used */
    MPI_Info info_out3;
    MPI_Info info_out4;
    MPI_Info info_out5;
    char new_key[MPI_MAX_INFO_KEY];

    /* Read arguments: info key and value */
    if (argc < 3) {
        fprintf(stdout, "Usage: %s -hint=[infokey] -value=[VALUE]\n", argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-hint=", strlen("-hint="))) {
                strncpy(query_key, argv[i] + strlen("-hint="), MPI_MAX_INFO_KEY);
            } else if (!strncmp(argv[i], "-value=", strlen("-value="))) {
                strncpy(val, argv[i] + strlen("-value="), MPI_MAX_INFO_KEY);
            }
        }
    }

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Test 1: comm_set_info and comm_get_info */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_set_info and MPI_Comm_get_info");
    MPI_Comm_dup(MPI_COMM_WORLD, &comm_dup1);
    MPI_Info_create(&info_in1);
    MPI_Info_set(info_in1, query_key, val);
    MPI_Comm_set_info(comm_dup1, info_in1);
    MPI_Comm_get_info(comm_dup1, &info_out1);

    errors += ReadCommInfo(info_out1, query_key, val, buf, "MPI_Comm_{set,get}_info");
    MPI_Info_free(&info_out1);
    /* Release comm_dup1 later, still using */

    /* Test 2: Comm_dup_with_info with comm=MPI_COMM_WORLD */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_dup_with_info with comm = MPI_COMM_WORLD");
    MPI_Comm_dup_with_info(MPI_COMM_WORLD, info_in1, &comm_dup3);
    MPI_Comm_get_info(comm_dup3, &info_out3);
    errors += ReadCommInfo(info_out3, query_key, val, buf, "MPI_Comm_dup_with_info-1");
    MPI_Info_free(&info_out3);
    /* Release comm_dup3 later, still using in a later test */

    /* Test 3: Comm_dup_with_info with comm = dup of MPI_COMM_WORLD */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_dup_with_info with comm = dup of MPI_COMM_WORLD");
    /* Pick a new hint that is different from the user provided hint */
    if (!strcmp(query_key, "mpi_assert_no_any_source"))
        snprintf(new_key, MPI_MAX_INFO_KEY, "mpi_assert_no_any_tag");
    else
        snprintf(new_key, MPI_MAX_INFO_KEY, "mpi_assert_no_any_source");
    MPI_Info_create(&info_in4);
    /* Add new hint */
    MPI_Info_set(info_in4, new_key, "true");
    MPI_Comm_dup_with_info(comm_dup3, info_in4, &comm_dup4);
    MPI_Comm_get_info(comm_dup4, &info_out4);
    errors += ReadCommInfo(info_out4, new_key, "true", buf, "MPI_Comm_dup_with_info-2a");
    MPI_Comm_free(&comm_dup3);
    MPI_Comm_free(&comm_dup4);
    MPI_Info_free(&info_in4);
    MPI_Info_free(&info_out4);

    /* Test 4: MPI_Comm_split_type */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, info_in1, &comm_split5);
    MPI_Comm_get_info(comm_split5, &info_out5);
    errors += ReadCommInfo(info_out5, query_key, val, buf, "MPI_Comm_split_type");
    MPI_Comm_free(&comm_split5);
    MPI_Info_free(&info_out5);

    /* Test comm_idup */
#if MPI_VERSION >= 4
    MPI_Request request;
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_idup_with_info with comm = MPI_COMM_WORLD");
    MPI_Comm_idup_with_info(MPI_COMM_WORLD, info_in1, &comm_dup3, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    MPI_Comm_get_info(comm_dup3, &info_out3);
    errors += ReadCommInfo(info_out3, query_key, val, buf, "MPI_Comm_idup_with_info");
    MPI_Info_free(&info_out3);
    MPI_Comm_free(&comm_dup3);
#endif

    /* TODO - Test: MPI_Dist_graph_create_adjacent */

    /* Release remaining resources */
    MPI_Comm_free(&comm_dup1);
    MPI_Info_free(&info_in1);

    MTest_Finalize(errors);
    return MTestReturnValue(errs);
}
