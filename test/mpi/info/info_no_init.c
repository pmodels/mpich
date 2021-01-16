/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitest.h"

#define NUM_INFOS 128

int modify_info(MPI_Info * infos, const char *get_key, const char *get_value,
                const char *set_key, const char *set_value);

int main(int argc, char *argv[])
{
    MPI_Info infos[NUM_INFOS];
    int i, nerrs = 0;

    /* MPI_Info functions can be called before MPI_Init(). */
    for (i = 0; i < NUM_INFOS; i++)
        MPI_Info_create(&infos[i]);

    /* Modify some. */
    nerrs += modify_info(infos, NULL, NULL, "key1", "value1");
    nerrs += modify_info(infos, "key1", "value1", "key2", "value2");

    MPI_Init(&argc, &argv);

    /* Modify some. */
    nerrs += modify_info(infos, "key2", "value2", "key3", "value3");

    MPI_Finalize();

    /* MPI_Info functions can be called after MPI_Finalize(). */

    /* Modify some. */
    nerrs += modify_info(infos, "key3", "value3", "key4", "value4");
    nerrs += modify_info(infos, "key4", "value4", NULL, NULL);
    for (i = 0; i < NUM_INFOS; i++)
        MPI_Info_free(&infos[i]);

    /* Let's check if we can create all info objects now.  This checks if the
     * MPICH implementation internally cleans up the MPI_Info handle pool
     * properly when all handles are freed and that pool is still
     * reinitialized. */
    for (i = 0; i < NUM_INFOS; i++)
        MPI_Info_create(&infos[i]);
    for (i = 0; i < NUM_INFOS; i++)
        MPI_Info_free(&infos[i]);

    if (nerrs) {
        printf(" Found %d errors\n", nerrs);
    } else {
        printf(" No Errors\n");
    }
    fflush(stdout);

    return nerrs ? 1 : 0;;
}


int modify_info(MPI_Info * infos, const char *get_key, const char *get_value,
                const char *set_key, const char *set_value)
{
    int i, nerrs = 0;
    static int num_modified = 0;

    /* Duplicate some info objects to check if MPI_Info_dup() works.  Always
     * 50% will be duplicated and replaced. */
    for (i = 0; i < NUM_INFOS; i += (1 << (num_modified + 1))) {
        int j;
        for (j = i; j < i + (1 << num_modified); j++) {
            if (j >= NUM_INFOS)
                continue;
            MPI_Info newinfo;
            MPI_Info_dup(infos[i], &newinfo);
            /* infos[i] is not needed. */
            MPI_Info_free(&infos[i]);
            /* newinfo becomes a new infos[i]. */
            infos[i] = newinfo;
        }
    }
    num_modified += 1;
    if ((1 << num_modified) >= NUM_INFOS) {
        num_modified = 0;
    }

    /* Get values. */
    if (get_key && get_value) {
        for (i = 0; i < NUM_INFOS; i++) {
            int valuelen = MPI_MAX_INFO_VAL, flag;
            char value[MPI_MAX_INFO_VAL];
            MPI_Info_get_string(infos[i], get_key, &valuelen, value, &flag);
            if (!flag) {
                /* infos[i] should have get_key:get_value pair */
                nerrs++;
            } else if (strcmp(value, get_value) != 0) {
                /* infos[i] should have get_key:get_value pair */
                nerrs++;
            }
        }
    }

    /* Create new info objects to check if MPI_Info_create() works. Always 50%
     * will be newly created and replaced. */
    for (i = 0; i < NUM_INFOS; i += (1 << (num_modified + 1))) {
        int j;
        for (j = i; j < i + (1 << num_modified); j++) {
            if (j >= NUM_INFOS)
                continue;
            MPI_Info_free(&infos[i]);
            MPI_Info_create(&infos[i]);
        }
    }
    num_modified += 1;
    if ((1 << num_modified) >= NUM_INFOS) {
        num_modified = 0;
    }

    /* Set values. */
    if (set_key && set_value) {
        for (i = 0; i < NUM_INFOS; i++) {
            MPI_Info_set(infos[i], set_key, set_value);
        }
    }

    return nerrs;
}
