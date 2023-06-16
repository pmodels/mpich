/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* This is adapted example 10.9 from MPI Standard 4.0
 * This example illustrates the use of Process Set query functions to select a
 * Process Set to use for MPI Group creation. First, the default error handler
 * can be specified when instatiating a Session instance. Second, there must be
 * at least two process sets associated with a Session. Third, the example
 * illustrates use of session info object and the one required key: "mpi_size".
 */

int main(int argc, char *argv[])
{
    int errs = 0;
    int n_psets, psetlen, rc;
    int valuelen;
    int flag = 0;
    char *info_val = NULL;
    char *pset_name = NULL;
    char **all_pset_names;
    MPI_Session shandle = MPI_SESSION_NULL;
    MPI_Info sinfo = MPI_INFO_NULL;
    MPI_Group pgroup = MPI_GROUP_NULL;

    rc = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &shandle);
    if (rc != MPI_SUCCESS) {
        printf("Could not initialize session, bailing out\n");
        errs++;
        return MTestReturnValue(errs);
    }

    MPI_Session_get_num_psets(shandle, MPI_INFO_NULL, &n_psets);
    all_pset_names = malloc(n_psets * sizeof(char *));
    assert(all_pset_names);

    for (int i = 0; i < n_psets; i++) {
        psetlen = 0;
        MPI_Session_get_nth_pset(shandle, MPI_INFO_NULL, i, &psetlen, NULL);
        if (psetlen <= 0) {
            printf("MPI_Session_get_nth_pset: returns psetlen = %d\n", psetlen);
            errs++;
        }

        pset_name = (char *) malloc(sizeof(char) * psetlen);
        MPI_Session_get_nth_pset(shandle, MPI_INFO_NULL, i, &psetlen, pset_name);
        if (strlen(pset_name) + 1 != psetlen) {
            printf("MPI_Session_get_nth_pset: strlen(\"%s\") + 1 != %d\n", pset_name, psetlen);
            errs++;
        }

        all_pset_names[i] = strdup(pset_name);

        free(pset_name);
        pset_name = NULL;
    }

    bool got_world = false;
    bool got_self = false;
    for (int i = 0; i < n_psets; i++) {
        /* get instance of an info object for this Session */
        MPI_Session_get_pset_info(shandle, all_pset_names[i], &sinfo);
        MPI_Info_get_valuelen(sinfo, "mpi_size", &valuelen, &flag);
        if (valuelen <= 0) {
            printf("MPI_Session_get_pset_info: valuelen of mpi_size is %d\n", valuelen);
            errs++;
        }
        info_val = (char *) malloc(valuelen + 1);
        MPI_Info_get(sinfo, "mpi_size", valuelen, info_val, &flag);
        if (atol(info_val) <= 0) {
            printf("\"mpi_size\" of %s proc set is %s\n", all_pset_names[i], info_val);
        }
        if (strcmp("mpi://WORLD", all_pset_names[i]) == 0) {
            got_world = true;
        }
        if (strcmp("mpi://SELF", all_pset_names[i]) == 0) {
            got_self = true;
            if (atol(info_val) != 1) {
                printf("\"mpi_size\" of SELF proc set is %s\n", info_val);
            }
        }
        free(info_val);

        /* create a group from the process set */
        rc = MPI_Group_from_session_pset(shandle, all_pset_names[i], &pgroup);
        if (rc != MPI_SUCCESS) {
            printf("Could not create group from pset %s\n", all_pset_names[i]);
            errs++;
        }

        free(all_pset_names[i]);
        MPI_Group_free(&pgroup);
        MPI_Info_free(&sinfo);
    }
    free(all_pset_names);
    MPI_Session_finalize(&shandle);

    if (!got_self) {
        printf("proc set mpi://SELF not found\n");
        errs++;
    }
    if (!got_world) {
        printf("proc set mpi://WORLD not found\n");
        errs++;
    }

    if (errs == 0) {
        printf("No Errors\n");
    } else {
        printf("%d Errors\n", errs);
    }
    return MTestReturnValue(errs);
}
