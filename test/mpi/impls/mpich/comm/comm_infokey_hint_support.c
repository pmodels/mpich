/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
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
int ReadCommInfo(MPI_Info info, const char *key, const char *val, char *buf, const char *test_name)
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
    int i, j, rank;
    int errors = 0, errs = 0;
    char query_key[MPI_MAX_INFO_KEY];
    char buf[MPI_MAX_INFO_VAL];
    char val[MPI_MAX_INFO_VAL];
    bool hint_mutable;          /* a hint is mutable or immutable based on an MPI implementation */
    MPI_Comm comm_dup1;
    MPI_Comm comm_dup2;
    MPI_Comm comm_dup3;
    MPI_Comm comm_dup4;
    MPI_Comm comm_split5;
    MPI_Info info_in1;
    MPI_Info info_in4;
    MPI_Info info_out1;
    MPI_Info info_out2;
    MPI_Info info_out3;
    MPI_Info info_out4;
    MPI_Info info_out5;
    char new_key[MPI_MAX_INFO_KEY];

    /* Read arguments: info key and value */
    if (argc < 4) {
        fprintf(stdout,
                "Usage: %s -comminfohint=[infokey] -value=[VALUE] -hintmutable=[true/false]\n",
                argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-comminfohint=", strlen("-comminfohint="))) {
                snprintf(query_key, MPI_MAX_INFO_KEY, argv[i] + strlen("-comminfohint="));
            } else if (!strncmp(argv[i], "-value=", strlen("-value="))) {
                snprintf(val, MPI_MAX_INFO_VAL, argv[i] + strlen("-value="));
            } else if (!strncmp(argv[i], "-hintmutable=", strlen("-hintmutable="))) {
                char is_hint_mutable[10];
                snprintf(is_hint_mutable, 10, argv[i] + strlen("-hintmutable="));
                if (!strcmp(is_hint_mutable, "true"))
                    hint_mutable = true;
                else
                    hint_mutable = false;
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

    /* Validate the value only if the hint is mutable */
    /* For immutable, the supplied value is now ignored by MPI_comm_set_info */
    if (hint_mutable)
        errors += ReadCommInfo(info_out1, query_key, val, buf, "MPI_Comm_{set,get}_info");
    MPI_Info_free(&info_out1);
    /* Release comm_dup1 later, still using */

    /* Test 2: comm_dup */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_dup: source comm has user provided comm infohint");
    MPI_Comm_dup(comm_dup1, &comm_dup2);
    MPI_Comm_get_info(comm_dup2, &info_out2);
    MPI_Comm_free(&comm_dup2);
    /* Note: For MPICH, we currently expect MPI_Comm_dup to copy hints from source
     * to dup comm. Such copying with not be done after we align behavior of MPICH
     * with the new MPI standard (MPI 3.2 onwards). */
    /* If the hint is immutable, user provided value is not set in comm,
     * so skip the validation */
    if (hint_mutable)
        errors += ReadCommInfo(info_out2, query_key, val, buf, "MPI_Comm_dup");
    MPI_Info_free(&info_out2);

    /* Test 3: Comm_dup_with_info with comm=MPI_COMM_WORLD */
    if (rank == 0)
        MTestPrintfMsg(1, "Testing MPI_Comm_dup_with_info with comm = MPI_COMM_WORLD");
    MPI_Comm_dup_with_info(MPI_COMM_WORLD, info_in1, &comm_dup3);
    MPI_Comm_get_info(comm_dup3, &info_out3);
    errors += ReadCommInfo(info_out3, query_key, val, buf, "MPI_Comm_dup_with_info-1");
    MPI_Info_free(&info_out3);
    /* Release comm_dup3 later, still using in a later test */

    /* Test 4: Comm_dup_with_info with comm = dup of MPI_COMM_WORLD */
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
    /* Note: Currently we expect hints to be copied from comm. When MPICH aligns with
     * MPI 3.2+,we do not expect such copying from comm to comm_dup4 */
    errors += ReadCommInfo(info_out4, query_key, val, buf, "MPI_Comm_dup_with_info-2b");
    MPI_Comm_free(&comm_dup3);
    MPI_Comm_free(&comm_dup4);
    MPI_Info_free(&info_in4);
    MPI_Info_free(&info_out4);

    /* Test 5: MPI_Comm_split_type */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, info_in1, &comm_split5);
    MPI_Comm_get_info(comm_split5, &info_out5);
    errors += ReadCommInfo(info_out5, query_key, val, buf, "MPI_Comm_split_type");
    MPI_Comm_free(&comm_split5);
    MPI_Info_free(&info_out5);

    /* TODO - Test: MPI_Dist_graph_create_adjacent */

    /* Release remaining resources */
    MPI_Comm_free(&comm_dup1);
    MPI_Info_free(&info_in1);

    MTest_Finalize(errors);
    return MTestReturnValue(errs);
}
