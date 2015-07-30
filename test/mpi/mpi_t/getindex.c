/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Test MPI_T_xxx_get_index() for cvars, pvars and categories.
 */
#include <stdio.h>
#include "mpi.h"

static int verbose = 0;

int main(int argc, char *argv[])
{
    int i;
    int required, provided, namelen;
    int num_cvar, num_pvar, num_cat;
    int cvar_index, pvar_index, cat_index;
    int pvar_class;
    char name[128];
    int errno, errs = 0;

    required = MPI_THREAD_SINGLE;
    MPI_T_init_thread(required, &provided);
    MPI_Init(&argc, &argv);

    /* Test MPI_T_cvar_get_index with both valid and bogus names */
    MPI_T_cvar_get_num(&num_cvar);
    if (verbose)
        fprintf(stdout, "%d MPI Control Variables\n", num_cvar);
    for (i = 0; i < num_cvar; i++) {
        namelen = sizeof(name);
        MPI_T_cvar_get_info(i, name, &namelen, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (namelen <= 128) {
            errno = MPI_T_cvar_get_index(name, &cvar_index);
            if (errno != MPI_SUCCESS || cvar_index != i)
                errs++;
        }
    }
    errno = MPI_T_cvar_get_index("AN INVALID CVAR NAME FOR TEST", &cvar_index);
    if (errno != MPI_T_ERR_INVALID_NAME)
        errs++;

    if (errs)
        fprintf(stdout, "Errors found in MPI_T_cvar_get_index\n");

    /* Test MPI_T_pvar_get_index with both valid and bogus names */
    MPI_T_pvar_get_num(&num_pvar);
    if (verbose)
        fprintf(stdout, "%d MPI Performance Variables\n", num_pvar);

    for (i = 0; i < num_pvar; i++) {
        namelen = sizeof(name);
        MPI_T_pvar_get_info(i, name, &namelen, NULL, &pvar_class, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL, NULL);
        if (namelen <= 128) {
            errno = MPI_T_pvar_get_index(name, pvar_class, &pvar_index);
            if (errno != MPI_SUCCESS || pvar_index != i)
                errs++;
        }
    }
    errno =
        MPI_T_pvar_get_index("AN INVALID PVAR NAME FOR TEST", MPI_T_PVAR_CLASS_COUNTER,
                             &cvar_index);
    if (errno != MPI_T_ERR_INVALID_NAME)
        errs++;
    if (errs)
        fprintf(stdout, "Errors found in MPI_T_cvar_get_index\n");

    /* Test MPI_T_category_get_index with both valid and bogus names */
    MPI_T_category_get_num(&num_cat);
    if (verbose)
        fprintf(stdout, "%d MPI_T categories\n", num_cat);
    for (i = 0; i < num_cat; i++) {
        namelen = sizeof(name);
        MPI_T_category_get_info(i, name, &namelen, NULL, NULL, NULL, NULL, NULL);
        if (namelen <= 128) {
            errno = MPI_T_category_get_index(name, &cat_index);
            if (errno != MPI_SUCCESS || cat_index != i)
                errs++;
        }
    }
    errno = MPI_T_category_get_index("AN INVALID CATEGORY NAME FOR TEST", &cat_index);
    if (errno != MPI_T_ERR_INVALID_NAME)
        errs++;
    if (errs)
        fprintf(stdout, "Errors found in MPI_T_cvar_get_index\n");

    MPI_T_finalize();
    MPI_Finalize();

    if (errs == 0)
        fprintf(stdout, " No Errors\n");
    return 0;
}
