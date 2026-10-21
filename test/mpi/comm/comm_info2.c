/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_comm_info2
int run(const char *arg);
#endif

/*
  This program provides tests cases for the functional evaluation
  of the MPI communicator info hints infrastructure.
 */

int run(const char *arg)
{
    int rank;
    MPI_Info info_in, info_out;
    int errs = 0;
    MPI_Comm comm;
    char query_key[MPI_MAX_INFO_KEY];
    char val[MPI_MAX_INFO_VAL];
    char new_key[MPI_MAX_INFO_KEY];
    MPI_Comm comm_dup2;
    MPI_Comm comm_dup3;
    MPI_Comm comm_dup4;
    MPI_Comm comm_split1;
    MPI_Info info_out2;
    MPI_Info info_out3;
    MPI_Info info_out5;
    MPI_Info info_in3;

    /* Read arguments: info key and value */
    snprintf(query_key, MPI_MAX_INFO_KEY, "arbitrary_key");
    snprintf(val, MPI_MAX_INFO_VAL, "arbitrary_val");

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Goal of the test is only to make sure that MPI does not break */
    /* No expectation is made on if the hints are actually set */

    /* Test 1: comm_dup */
    if (rank == 0)
        MTestPrintfMsg(1,
                       "Testing MPI_Comm_dup with a source comm that has default comm infohints");
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    /* Test 2: comm_set_info and comm_get_info */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_set_info and MPI_Comm_get_info");
    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, query_key, val);
    MPI_Comm_set_info(comm, info_in);
    MPI_Comm_get_info(comm, &info_out);
    MPI_Info_free(&info_out);

    /* Test 3: com_dup after info is set in source comm */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_dup: source comm has user provided comm infohint");
    MPI_Comm_dup(comm, &comm_dup2);
    MPI_Comm_get_info(comm_dup2, &info_out2);
    MPI_Comm_free(&comm_dup2);
    MPI_Info_free(&info_out2);

    /* Test 4: Comm_dup_with_info */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_dup_with_info");

    /* Add 2 hints and create comm dup with info */
    MPI_Info_create(&info_in3);
    MPI_Info_set(info_in3, query_key, val);
    snprintf(new_key, MPI_MAX_INFO_KEY, "arbitrary_key_2");
    MPI_Info_set(info_in3, new_key, "arbitrary_value_2");
    MPI_Comm_dup_with_info(comm, info_in3, &comm_dup3);
    MPI_Comm_get_info(comm_dup3, &info_out3);
    MPI_Info_free(&info_in3);
    MPI_Info_free(&info_out3);
    MPI_Comm_free(&comm_dup3);

    /* Test 5: MPI_Comm_split_type */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, info_in, &comm_split1);
    MPI_Comm_get_info(comm_split1, &info_out5);
    MPI_Info_free(&info_out5);
    MPI_Comm_free(&comm_split1);

    /* Test 6: comm_dup_with_info with info=MPI_INFO_NULL */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_dup_with_info with MPI_INFO_NULL");
    MPI_Comm_dup_with_info(comm, MPI_INFO_NULL, &comm_dup4);
    MPI_Comm_free(&comm_dup4);

    /* TODO - Test: MPI_Dist_graph_create_adjacent */

    /* Release remaining resources */
    MPI_Info_free(&info_in);
    MPI_Comm_free(&comm);

    return errs;
}
