/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static const char *memory_alloc_kinds[3][5] = {
    /* mpi 4.1 kinds */
    {"mpi", "alloc_mem", "win_allocate", "win_allocate_shared", NULL},
    {"system", NULL},
    {NULL}
};

static bool is_supported(const char *kind);

int MPIR_get_supported_memory_kinds(char *requested_kinds, char **out_kinds)
{
    char *tmp_strs[1024];
    int num = 2;
    /* mpi and system kinds are always supported */
    tmp_strs[0] = MPL_strdup("mpi");
    tmp_strs[1] = MPL_strdup("system");

    /* match user kinds with supported kinds */
    if (requested_kinds != NULL) {
        char *tmp = MPL_strdup(requested_kinds);
        char *save_tmp = tmp;
        for (char *kind = MPL_strsep(&tmp, ","); kind; kind = MPL_strsep(&tmp, ",")) {
            if (!MPL_stricmp(kind, "mpi") || !MPL_stricmp(kind, "system")) {
                continue;
            } else if (is_supported(kind)) {
                tmp_strs[num++] = MPL_strdup(kind);
                /* FIXME: use dynamic storage */
                MPIR_Assert(num < 1024);
            }
        }
        MPL_free(save_tmp);
    }

    /* build return string */
    *out_kinds = MPL_strjoin(tmp_strs, num, ',');

    for (int i = 0; i < num; i++) {
        MPL_free(tmp_strs[i]);
    }

    return MPI_SUCCESS;
}

/*** static functions ***/

static bool is_supported(const char *kind)
{
    bool ret = false;
    char *tmp = MPL_strdup(kind);
    char *save_tmp = tmp;

    char *k = MPL_strsep(&tmp, ":");
    for (int i = 0; memory_alloc_kinds[i][0]; i++) {
        if (!MPL_stricmp(k, memory_alloc_kinds[i][0])) {
            ret = true;

            /* check that any restrictors are also supported */
            char *res;
            while ((res = MPL_strsep(&tmp, ":")) != NULL) {
                bool res_supported = false;

                for (int j = 1; memory_alloc_kinds[i][j]; j++) {
                    if (!MPL_stricmp(res, memory_alloc_kinds[i][j])) {
                        res_supported = true;
                    }
                }

                ret = ret && res_supported;
            }
        }
    }

    MPL_free(save_tmp);
    return ret;
}
