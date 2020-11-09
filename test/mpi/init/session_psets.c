/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* This is adapted example 10.9 from MPI Standard 4.0 */

static char use_pset_name[] = "SELF";

int main(int argc, char *argv[])
{
    int errs = 0;
    int i, n_psets, psetlen, rc, ret;
    int valuelen;
    int flag = 0;
    char *pset_name = NULL;
    char *info_val = NULL;
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
    for (i = 0, pset_name = NULL; i < n_psets; i++) {
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

        if (strstr(pset_name, use_pset_name) != NULL)
            break;

        free(pset_name);
        pset_name = NULL;
    }

    /*
     * get instance of an info object for this Session
     */
    MPI_Session_get_pset_info(shandle, pset_name, &sinfo);
    MPI_Info_get_valuelen(sinfo, "mpi_size", &valuelen, &flag);
    if (valuelen <= 0) {
        printf("MPI_Session_get_pset_info: valuelen of mpi_size is %d\n", valuelen);
        errs++;
    }
    info_val = (char *) malloc(valuelen + 1);
    MPI_Info_get(sinfo, "mpi_size", valuelen, info_val, &flag);
    if (atol(info_val) != 1) {
        printf("\"mpi_size\" of SELF proc set is %s\n", info_val);
    }
    free(info_val);

    /*
     * create a group from the process set
     */
    rc = MPI_Group_from_session_pset(shandle, pset_name, &pgroup);
    if (rc != MPI_SUCCESS) {
        printf("Could not create grep from pset %s\n", pset_name);
        errs++;
    }

    free(pset_name);
    MPI_Group_free(&pgroup);
    MPI_Info_free(&sinfo);
    MPI_Session_finalize(&shandle);

    if (errs == 0) {
        printf("No Errors\n");
    } else {
        printf("%d Errors\n", errs);
    }
    return MTestReturnValue(errs);
}
