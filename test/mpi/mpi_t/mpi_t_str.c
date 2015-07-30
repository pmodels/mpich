/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* A test that MPI_T string handling is working as expected.  Necessarily a weak
 * test, since we can't assume any particular variables are exposed by the
 * implementation. */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include "mpitestconf.h"

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__); \
            }                                                                     \
        }                                                                         \
    } while (0)

/* the usual multiple-evaluation caveats apply to this routine */
#define min(a,b) ((a) < (b) ? (a) : (b))

int main(int argc, char **argv)
{
    int errs = 0;
    int i, j;
    int rank, size;
    int num_pvars, num_cvars, num_cat;
#define STR_SZ (50)
    int name_len;
    char name[STR_SZ + 1] = ""; /* +1 to check for overrun */
    int desc_len;
    char desc[STR_SZ + 1] = ""; /* +1 to check for overrun */
    int verb;
    MPI_Datatype dtype;
    int count;
    int bind;
    int scope;
    int provided;

    /* Init'ed to a garbage value, to trigger MPI_T bugs easily if there are. */
    MPI_T_enum enumtype = (MPI_T_enum) 0x31415926;

    MPI_Init(&argc, &argv);
    MPI_T_init_thread(MPI_THREAD_SINGLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* loop over all cvars and ask for string arguments with various valid
     * combinations of NULL and non-NULL to ensure that the library handles this
     * case correctly */
    MPI_T_cvar_get_num(&num_cvars);
    for (i = 0; i < num_cvars; ++i) {
        int full_name_len, full_desc_len;
        /* pass NULL string, non-zero lengths; should get full lengths */
        full_name_len = full_desc_len = 1;
        MPI_T_cvar_get_info(i, NULL, &full_name_len, &verb, &dtype,
                            &enumtype, NULL, &full_desc_len, &bind, &scope);
        check(full_name_len >= 0);
        check(full_desc_len >= 0);

        /* pass non-NULL string, zero lengths; should get full lengths also */
        name_len = desc_len = 0;
        MPI_T_cvar_get_info(i, name, &name_len, &verb, &dtype,
                            &enumtype, desc, &desc_len, &bind, &scope);
        check(full_name_len == name_len);
        check(full_desc_len == desc_len);

        /* regular call, no NULLs; should truncate (with termination) to STR_SZ
         * if necessary, otherwise returns strlen+1 in the corresponding "_len"
         * var */
        name_len = desc_len = STR_SZ;
        MPI_T_cvar_get_info(i, name, &name_len, &verb, &dtype,
                            &enumtype, desc, &desc_len, &bind, &scope);
        check((strlen(name) + 1) == min(name_len, STR_SZ));
        check((strlen(desc) + 1) == min(desc_len, STR_SZ));

        /* pass NULL lengths, string buffers should be left alone */
        for (j = 0; j < STR_SZ; ++j) {
            name[j] = j % CHAR_MAX;
            desc[j] = j % CHAR_MAX;
        }
        MPI_T_cvar_get_info(i, name, /*name_len= */ NULL, &verb, &dtype,
                            &enumtype, desc, /*desc_len= */ NULL, &bind, &scope);
        for (j = 0; j < STR_SZ; ++j) {
            check(name[j] == j % CHAR_MAX);
            check(desc[j] == j % CHAR_MAX);
        }

        /* not much of a string test, just need a quick spot to stick a test for
         * the existence of the correct MPI_T prototype (tt#1727) */
        /* Include test that enumtype is defined */
        if (dtype == MPI_INT && enumtype != MPI_T_ENUM_NULL) {
            int num_enumtype = -1;
            name_len = STR_SZ;
            MPI_T_enum_get_info(enumtype, &num_enumtype, name, &name_len);
            check(num_enumtype >= 0);
        }
    }

    /* check string handling for performance variables */
    MPI_T_pvar_get_num(&num_pvars);
    for (i = 0; i < num_pvars; ++i) {
        int varclass, bind, readonly, continuous, atomic;
        MPI_Datatype dtype;
        MPI_T_enum enumtype;

        int full_name_len, full_desc_len;
        /* pass NULL string, non-zero lengths; should get full lengths */
        full_name_len = full_desc_len = 1;
        MPI_T_pvar_get_info(i, NULL, &full_name_len, &verb, &varclass, &dtype,
                            &enumtype, NULL, &full_desc_len, &bind, &readonly,
                            &continuous, &atomic);
        check(full_name_len >= 0);
        check(full_desc_len >= 0);

        /* pass non-NULL string, zero lengths; should get full lengths also */
        name_len = desc_len = 0;
        MPI_T_pvar_get_info(i, name, &name_len, &verb, &varclass, &dtype,
                            &enumtype, desc, &desc_len, &bind, &readonly, &continuous, &atomic);
        check(full_name_len == name_len);
        check(full_desc_len == desc_len);

        /* regular call, no NULLs; should truncate (with termination) to STR_SZ
         * if necessary, otherwise returns strlen+1 in the corresponding "_len"
         * var */
        name[STR_SZ] = (char) 'Z';
        desc[STR_SZ] = (char) 'Z';
        name_len = desc_len = STR_SZ;
        MPI_T_pvar_get_info(i, name, &name_len, &verb, &varclass, &dtype,
                            &enumtype, desc, &desc_len, &bind, &readonly, &continuous, &atomic);
        check((strlen(name) + 1) == min(name_len, STR_SZ));
        check((strlen(desc) + 1) == min(desc_len, STR_SZ));
        check(name[STR_SZ] == (char) 'Z');
        check(desc[STR_SZ] == (char) 'Z');

        /* pass NULL lengths, string buffers should be left alone */
        for (j = 0; j < STR_SZ; ++j) {
            name[j] = j % CHAR_MAX;
            desc[j] = j % CHAR_MAX;
        }
        MPI_T_pvar_get_info(i, name, /*name_len= */ NULL, &verb, &varclass, &dtype,
                            &enumtype, desc, /*desc_len= */ NULL, &bind, &readonly,
                            &continuous, &atomic);
        for (j = 0; j < STR_SZ; ++j) {
            check(name[j] == j % CHAR_MAX);
            check(desc[j] == j % CHAR_MAX);
        }
    }

    /* check string handling for categories */
    MPI_T_category_get_num(&num_cat);
    for (i = 0; i < num_cat; ++i) {
        int full_name_len, full_desc_len;
        /* pass NULL string, non-zero lengths; should get full lengths */
        full_name_len = full_desc_len = 1;
        MPI_T_category_get_info(i, NULL, &full_name_len, NULL, &full_desc_len,
                                &num_cvars, &num_pvars, /*num_categories= */ &j);
        check(full_name_len >= 0);
        check(full_desc_len >= 0);

        /* pass non-NULL string, zero lengths; should get full lengths also */
        name_len = desc_len = 0;
        MPI_T_category_get_info(i, name, &name_len, desc, &desc_len,
                                &num_cvars, &num_pvars, /*num_categories= */ &j);
        check(full_name_len == name_len);
        check(full_desc_len == desc_len);

        /* regular call, no NULLs; should truncate (with termination) to STR_SZ
         * if necessary, otherwise returns strlen+1 in the corresponding "_len"
         * var */
        name[STR_SZ] = (char) 'Z';
        desc[STR_SZ] = (char) 'Z';
        name_len = desc_len = STR_SZ;
        MPI_T_category_get_info(i, name, &name_len, desc, &desc_len,
                                &num_cvars, &num_pvars, /*num_categories= */ &j);
        check((strlen(name) + 1) == min(name_len, STR_SZ));
        check((strlen(desc) + 1) == min(desc_len, STR_SZ));
        check(name[STR_SZ] == (char) 'Z');
        check(desc[STR_SZ] == (char) 'Z');

        /* pass NULL lengths, string buffers should be left alone */
        for (j = 0; j < STR_SZ; ++j) {
            name[j] = j % CHAR_MAX;
            desc[j] = j % CHAR_MAX;
        }
        MPI_T_category_get_info(i, name, /*name_len= */ NULL, desc,
                                /*desc_len= */ NULL, &num_cvars, &num_pvars,
                                /*num_categories= */ &j);
        for (j = 0; j < STR_SZ; ++j) {
            check(name[j] == j % CHAR_MAX);
            check(desc[j] == j % CHAR_MAX);
        }

        /* not really a string test, just need a quick spot to stick a test for the
         * existence of the correct MPI_T prototype (tt#1727) */
        {
            int indices[1];
            MPI_T_category_get_pvars(i, 1, indices);
        }
    }

    MPI_Allreduce(MPI_IN_PLACE, &errs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (rank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }

    MPI_T_finalize();
    MPI_Finalize();

    return 0;
}
