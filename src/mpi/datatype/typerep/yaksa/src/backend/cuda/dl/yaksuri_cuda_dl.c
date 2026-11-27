/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* CUDA init hook for a build with --enable-dlopen. The CUDA backend is in a
 * module that is loaded at runtime. If the module cannot be loaded, e.g. on a
 * node without the CUDA runtime, yaksa runs without the CUDA backend. */

/* for dladdr */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "yaksi.h"
#include "yaksuri.h"
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#define MAX_PATH_LEN 4096

int yaksuri_cuda_init_hook(yaksur_gpudriver_hooks_s ** hooks)
{
    int rc = YAKSA_SUCCESS;

    *hooks = NULL;

    /* The module is installed in the same directory as the library (or the
     * executable) that contains this function. It is the only place we look. */
    Dl_info info;
    if (!dladdr((void *) &yaksuri_cuda_init_hook, &info) || info.dli_fname == NULL) {
        goto fn_exit;
    }

    char module_path[MAX_PATH_LEN];
    const char *slash = strrchr(info.dli_fname, '/');
    int dir_len = slash ? (int) (slash - info.dli_fname) : 1;
    snprintf(module_path, MAX_PATH_LEN, "%.*s/%s", dir_len, slash ? info.dli_fname : ".",
             YAKSA_CUDA_MODULE_NAME);

    /* the module stays loaded for the lifetime of the process */
    void *handle = dlopen(module_path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        goto fn_exit;
    }

    yaksuri_cuda_module_init_fn module_init;
    module_init = (yaksuri_cuda_module_init_fn) dlsym(handle, YAKSURI_CUDA_MODULE_INIT_SYMBOL);
    if (module_init == NULL) {
        dlclose(handle);
        goto fn_exit;
    }

    yaksuri_cuda_host_s host;
    host.has_wait_kernel = yaksuri_global.has_wait_kernel;
    host.iov = yaksi_iov;
    rc = module_init(&host, hooks);

  fn_exit:
    return rc;
}
