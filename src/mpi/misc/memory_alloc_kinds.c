/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static const char *memory_alloc_kinds[6][5] = {
    /* mpi 4.1 kinds */
    {"mpi", "alloc_mem", "win_allocate", "win_allocate_shared", NULL},
    {"system", NULL},
    /* memory allocation kinds 1.0 */
    {"cuda", "host", "device", "managed", NULL},
    {"rocm", "host", "device", "managed", NULL},
    {"level_zero", "host", "device", "shared", NULL},
    {NULL}
};

static bool is_supported(const char *kind);
static void add_gpu_kinds(char *tmp_strs[], int *num);

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

    /* make sure GPU kinds are included, if supported */
    add_gpu_kinds(tmp_strs, &num);

    /* build return string */
    *out_kinds = MPL_strjoin(tmp_strs, num, ',');

    for (int i = 0; i < num; i++) {
        MPL_free(tmp_strs[i]);
    }

    return MPI_SUCCESS;
}

/*** static functions ***/

/* return true if the input kind is an unsupported GPU kind */
static bool is_unsupported_gpu(const char *kind)
{
    int gpu_type;
    int gpu_supported;

    if (strcmp(kind, "cuda") == 0) {
        gpu_type = MPIX_GPU_SUPPORT_CUDA;
    } else if (strcmp(kind, "rocm") == 0) {
        gpu_type = MPIX_GPU_SUPPORT_HIP;
    } else if (strcmp(kind, "level_zero") == 0) {
        gpu_type = MPIX_GPU_SUPPORT_ZE;
    } else {
        return false;
    }

    (void) MPIR_GPU_query_support_impl(gpu_type, &gpu_supported);
    return !gpu_supported;
}

static bool is_supported(const char *kind)
{
    bool ret = false;
    char *tmp = MPL_strdup(kind);
    char *save_tmp = tmp;

    char *k = MPL_strsep(&tmp, ":");

    if (is_unsupported_gpu(k)) {
        goto fn_exit;
    }

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

fn_exit:
    MPL_free(save_tmp);
    return ret;
}

void add_gpu_kinds(char *tmp_strs[], int *num)
{
    if (MPIR_CVAR_ENABLE_GPU) {
        MPL_gpu_type_t type;
        MPL_gpu_query_support(&type);

        const char *kind;
        if (type == MPL_GPU_TYPE_CUDA) {
            kind = "cuda";
        } else if (type == MPL_GPU_TYPE_HIP) {
            kind = "rocm";
        } else if (type == MPL_GPU_TYPE_ZE) {
            kind = "level_zero";
        } else {
            return;
        }

        for (int i = 0; i < *num; i++) {
            if (strcmp(tmp_strs[i], kind) == 0) {
                return;
            }
        }
        tmp_strs[(*num)++] = MPL_strdup(kind);
        MPIR_Assert(*num < 1024);
    }
}
