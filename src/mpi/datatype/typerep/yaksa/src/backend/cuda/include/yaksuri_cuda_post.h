/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_CUDA_POST_H_INCLUDED
#define YAKSURI_CUDA_POST_H_INCLUDED

int yaksuri_cuda_init_hook(yaksur_gpudriver_hooks_s ** hooks);

#ifdef YAKSA_CUDA_DLOPEN
/* The CUDA backend is not part of libyaksa. It is built into a module that is
 * dlopen-ed at runtime by yaksuri_cuda_init_hook (see dl/yaksuri_cuda_dl.c).
 * The module does not link with libyaksa, so what the backend needs from the
 * rest of yaksa is passed to the entry function of the module. */
typedef struct {
    bool has_wait_kernel;
    int (*iov) (const char *buf, uintptr_t count, struct yaksi_type_s * type,
                uintptr_t iov_offset, struct iovec * iov, uintptr_t max_iov_len,
                uintptr_t * actual_iov_len);
} yaksuri_cuda_host_s;

#define YAKSURI_CUDA_MODULE_INIT_SYMBOL "yaksuri_cuda_module_init"
typedef int (*yaksuri_cuda_module_init_fn) (const yaksuri_cuda_host_s * host,
                                            yaksur_gpudriver_hooks_s ** hooks);
#endif

#endif /* YAKSURI_CUDA_H_INCLUDED */
