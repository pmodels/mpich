/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

/* This is example 10.8 from MPI Standard 4.0
 * Simple example illustrating creation of an MPI communicator using the Session
 * Model. The pre-defined "mpi://WORLD" process set can be used to first create
 * a local MPI group and then subsequently to create an MPI communicator from
 * this group.
 *
 * Different variants of this test:
 *
 * MULT_INIT:   Multiple MPI_Session_init/finalize sequences in a row (not nested)
 * WITH_WORLD:  Have MPI world model initialized prior to the first MPI_Session_init
 * RE_INIT:     Initialize another session after closing the previous one (only if
 *              not using MULT_INIT)
 * MOD_GROUP:   Create the first communicator from an MPI Group that is smaller
 *              than the world group
 */

int errs = 0;

int library_foo_test(void);
void library_foo_init(void);
void library_foo_finalize(void);

int main(int argc, char *argv[])
{
#ifdef MULT_INIT
    int rank, size, provided;
    int num_repeat = 1;

    if (argc > 1) {
        num_repeat = atoi(argv[1]);
        /* basic sanity check */
        assert(num_repeat > 0 && num_repeat < 100);
    }
#ifdef WITH_WORLD
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
    for (int i = 0; i < num_repeat; i++) {
#ifdef WITH_WORLD
        library_foo_test();
#else
        rank = library_foo_test();
#endif
        if (errs > 0) {
            break;
        }
    }
#ifdef WITH_WORLD
    MPI_Finalize();
#endif
#else
    int rank = library_foo_test();
#ifdef RE_INIT
    if (errs > 0) {
        goto fn_exit;
    }
    library_foo_test();
#endif
#endif
    if (rank == 0 && errs == 0) {
        printf("No Errors\n");
    }
  fn_exit:
    return MTestReturnValue(errs);
}

static MPI_Session lib_shandle = MPI_SESSION_NULL;
static MPI_Comm lib_comm = MPI_COMM_NULL;
#ifdef MOD_GROUP
static MPI_Comm lib_comm_group = MPI_COMM_NULL;
#endif

int check_thread_level(char *value)
{
    int ret = MPI_SUCCESS;

    if (strcmp(value, "MPI_THREAD_MULTIPLE") &&
        strcmp(value, "MPI_THREAD_SERIALIZED") &&
        strcmp(value, "MPI_THREAD_SINGLE") && strcmp(value, "MPI_THREAD_FUNNELED")) {

        ret = MPI_ERR_OTHER;
    }

    return ret;
}

int library_foo_test(void)
{
    int rank, size;
    int rc;

    library_foo_init();

    if (errs > 0) {
        rank = -1;
        goto fn_exit;
    }

    MPI_Comm_size(lib_comm, &size);
    MPI_Comm_rank(lib_comm, &rank);

    int sum;
#ifdef MOD_GROUP
    /* Skip the reduce operation for rank that is not in smaller group/ comm */
    if (lib_comm_group != MPI_COMM_NULL) {
        int group_comm_size;
        int group_comm_rank;

        MPI_Comm_size(lib_comm_group, &group_comm_size);
        MPI_Comm_rank(lib_comm_group, &group_comm_rank);

        rc = MPI_Reduce(&group_comm_rank, &sum, 1, MPI_INT, MPI_SUM, 0, lib_comm_group);
        if (rc != MPI_SUCCESS) {
            printf("Error on reduce\n");
            errs++;
        }
        if (group_comm_rank == 0) {
            if (sum != (group_comm_size - 1) * group_comm_size / 2) {
                printf("MPI_Reduce: expect %d, got %d\n",
                       (group_comm_size - 1) * group_comm_size / 2, sum);
                errs++;
            }
        }
    }
#endif
    sum = 0;
    rc = MPI_Reduce(&rank, &sum, 1, MPI_INT, MPI_SUM, 0, lib_comm);
    if (rc != MPI_SUCCESS) {
        printf("Error on reduce\n");
        errs++;
    }
    if (rank == 0) {
        if (sum != (size - 1) * size / 2) {
            printf("MPI_Reduce: expect %d, got %d\n", (size - 1) * size / 2, sum);
            errs++;
        }
    }

  fn_exit:
    library_foo_finalize();

    return rank;
}

void library_foo_init(void)
{
    int rc, flag;
    const char pset_name[] = "mpi://WORLD";
    const char mt_key[] = "thread_level";
    const char mt_value[] = "MPI_THREAD_MULTIPLE";
    char out_value[100];        /* large enough */

    MPI_Group wgroup = MPI_GROUP_NULL;
    MPI_Info sinfo = MPI_INFO_NULL;
    MPI_Info tinfo = MPI_INFO_NULL;
    MPI_Info_create(&sinfo);
    MPI_Info_set(sinfo, mt_key, mt_value);
    rc = MPI_Session_init(sinfo, MPI_ERRORS_RETURN, &lib_shandle);
    if (rc != MPI_SUCCESS) {
        printf("Error on MPI_Session_init\n");
        errs++;
        goto fn_exit;
    }

    /* check we got thread support level foo library needs */
    rc = MPI_Session_get_info(lib_shandle, &tinfo);
    if (rc != MPI_SUCCESS) {
        printf("Error on MPI_Session_get_info\n");
        errs++;
        goto fn_exit;
    }

    MPI_Info_get(tinfo, mt_key, sizeof(out_value), out_value, &flag);
    if (!flag) {
        printf("Could not find key %s\n", mt_key);
        errs++;
        goto fn_exit;
    }
    if (check_thread_level(out_value)) {
        printf("Did not get valid thread level, got %s\n", out_value);
        errs++;
        goto fn_exit;
    }

    /* create a group from the WORLD process set */
    rc = MPI_Group_from_session_pset(lib_shandle, pset_name, &wgroup);
    if (rc != MPI_SUCCESS) {
        printf("Error on MPI_Group_from_session_pset\n");
        errs++;
        goto fn_exit;
    }
#ifdef MOD_GROUP
    int grp_size, grp_rank, newgrp_size, newgrp_rank, comm_size = 0;
    MPI_Group newgroup = MPI_GROUP_NULL;
    MPI_Group_size(wgroup, &grp_size);
    MPI_Group_rank(wgroup, &grp_rank);
    int excl[1];

    if (grp_size < 2) {
        printf("This test has to be started with at least 2 processes!\n");
        errs++;
        goto fn_exit;
    }

    /* Create a new group where a rank is excluded */
    excl[0] = grp_size - 2;
    rc = MPI_Group_excl(wgroup, 1, excl, &newgroup);
    if (rc != MPI_SUCCESS) {
        printf("Error on MPI_Group_excl\n");
        errs++;
        goto fn_exit;
    }

    MPI_Group_size(newgroup, &newgrp_size);
    MPI_Group_rank(newgroup, &newgrp_rank);
    if (newgrp_size != grp_size - 1) {
        printf("Error: newgrp_size = %d, expected %d\n", newgrp_size, grp_size - 1);
        errs++;
        goto fn_exit;
    }

    /* Use the new smaller group to create the lib_comm_group */
    if (newgrp_rank == MPI_UNDEFINED) {
        rc = MPI_Comm_create_from_group(MPI_GROUP_EMPTY, "group", MPI_INFO_NULL,
                                        MPI_ERRORS_RETURN, &lib_comm_group);
        if (lib_comm_group != MPI_COMM_NULL) {
            printf
                ("Error: Expected comm from group MPI_GROUP_EMPTY to be MPI_COMM_NULL, but it is not.\n");
            errs++;
            goto fn_exit;
        }
    } else {
        rc = MPI_Comm_create_from_group(newgroup, "group", MPI_INFO_NULL,
                                        MPI_ERRORS_RETURN, &lib_comm_group);
        if (lib_comm_group == MPI_COMM_NULL) {
            printf
                ("Error: Expected comm from non-NULL group not to be MPI_COMM_NULL, but it is.\n");
            errs++;
            goto fn_exit;
        }
    }
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    if (lib_comm_group != MPI_COMM_NULL) {
        MPI_Comm_size(lib_comm_group, &comm_size);
        if (comm_size != newgrp_size) {
            printf("Error: communicator for smaller group has size %d (expected %d)\n",
                   comm_size, newgrp_size);
            errs++;
            goto fn_exit;
        }
    } else {
        /* Only for the rank that has been excluded from the group it is ok to have MPI_COMM_NULL */
        if (grp_rank != grp_size - 2) {
            errs++;
            printf("Error: communicator for smaller group is MPI_COMM_NULL\n");
            goto fn_exit;
        }
    }

    if (newgroup != MPI_GROUP_NULL) {
        MPI_Group_free(&newgroup);
    }

  mod_group_end:
#endif
    /* get a communicator */
    rc = MPI_Comm_create_from_group(wgroup, "org.mpi-forum.mpi-v4_0.example-ex10_8",
                                    MPI_INFO_NULL, MPI_ERRORS_RETURN, &lib_comm);
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    /* free group, library doesn’t need it. */
  fn_exit:
    if (wgroup != MPI_GROUP_NULL) {
        MPI_Group_free(&wgroup);
    }
    if (sinfo != MPI_INFO_NULL) {
        MPI_Info_free(&sinfo);
    }
    if (tinfo != MPI_INFO_NULL) {
        MPI_Info_free(&tinfo);
    }
}

void library_foo_finalize(void)
{
    int rc;
    if (lib_comm != MPI_COMM_NULL) {
        rc = MPI_Comm_free(&lib_comm);
        if (rc != MPI_SUCCESS) {
            printf("MPI_Comm_free returned %d\n", rc);
            errs++;
            return;
        }
    }
#ifdef MOD_GROUP
    if (lib_comm_group != MPI_COMM_NULL) {
        rc = MPI_Comm_free(&lib_comm_group);
        if (rc != MPI_SUCCESS) {
            printf("MPI_Comm_free returned %d\n", rc);
            errs++;
            return;
        }
    }
#endif

    if (lib_shandle != MPI_SESSION_NULL) {
        rc = MPI_Session_finalize(&lib_shandle);
        if (rc != MPI_SUCCESS) {
            printf("MPI_Session_finalize returned %d\n", rc);
            errs++;
            return;
        }
    }
}
