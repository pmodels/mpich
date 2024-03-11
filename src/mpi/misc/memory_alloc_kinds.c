/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* mpi 4.1 kinds */
static const char *mpi_kinds[5] =
    { "mpi", "alloc_mem", "win_allocate", "win_allocate_shared", NULL };
static const char *system_kinds[2] = { "system", NULL };

/* memory allocation kinds side doc 1.0 */
static const char *cuda_kinds[5] = { "cuda", "host", "device", "managed", NULL };
static const char *rocm_kinds[5] = { "rocm", "host", "device", "managed", NULL };
static const char *ze_kinds[5] = { "level_zero", "host", "device", "shared", NULL };

/* we support a maximum of 3 kinds at a time: mpi,system,gpu */
static const char **memory_alloc_kinds[4] = { mpi_kinds, system_kinds, NULL, NULL };

static bool is_supported(const char *kind);
static void init_gpu_kinds(void);

int MPIR_get_supported_memory_kinds(char *requested_kinds, char **out_kinds)
{
    static bool gpu_init_done = 0;
    char *tmp_strs[1024];
    int num = 0;

    if (!gpu_init_done) {
        init_gpu_kinds();
    }

    /* add kinds without restrictors */
    for (int i = 0; memory_alloc_kinds[i]; i++) {
        tmp_strs[i] = MPL_strdup(memory_alloc_kinds[i][0]);
        num++;
    }

    /* match user requested kinds with supported kinds */
    if (requested_kinds != NULL) {
        char *tmp = MPL_strdup(requested_kinds);
        char *save_tmp = tmp;
        for (char *kind = MPL_strsep(&tmp, ","); kind; kind = MPL_strsep(&tmp, ",")) {
            if (is_supported(kind)) {
                /* check if kind is already in supported list */
                bool found = false;
                for (int i = 0; i < num; i++) {
                    if (MPL_stricmp(kind, tmp_strs[i]) == 0) {
                        found = true;
                    }
                }

                if (!found) {
                    tmp_strs[num++] = MPL_strdup(kind);
                    /* FIXME: use dynamic storage */
                    MPIR_Assert(num < 1024);
                }
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

/* Requested kinds are of the form <kind>[:restrictor1][:restrictor2][...]
 * MPICH does not differentiate any of the defined restrictors. Thus, if the
 * user asks for a memory kind with one or more restrictors, we only check to
 * see if those restrictors are valid. For example:
 *
 * supported:     cuda
 * supported:     cuda:device
 * supported:     cuda:device:managed
 * supported:     mpi:alloc_mem
 * supported:     rocm
 * not supported: cuda:dev
 * not supported: mpi:host
 * not supported: system:foo
 */
static bool is_supported(const char *kind)
{
    bool ret = false;
    char *tmp = MPL_strdup(kind);
    char *save_tmp = tmp;

    char *k = MPL_strsep(&tmp, ":");

    for (int i = 0; memory_alloc_kinds[i]; i++) {
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

void init_gpu_kinds(void)
{
    if (MPIR_CVAR_ENABLE_GPU) {
        MPL_gpu_type_t type;
        MPL_gpu_query_support(&type);

        if (type == MPL_GPU_TYPE_CUDA) {
            memory_alloc_kinds[2] = cuda_kinds;
        } else if (type == MPL_GPU_TYPE_HIP) {
            memory_alloc_kinds[2] = rocm_kinds;
        } else if (type == MPL_GPU_TYPE_ZE) {
            memory_alloc_kinds[2] = ze_kinds;
        }
    }
}
