/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_CUDAI_H_INCLUDED
#define YAKSURI_CUDAI_H_INCLUDED

#include "yaksi.h"
#include <stdint.h>
#include <pthread.h>
#include <cuda_runtime_api.h>

#define CUDA_P2P_ENABLED  (1)
#define CUDA_P2P_DISABLED (2)
#define CUDA_P2P_CLIQUES  (3)

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#include <yaksuri_cudai_base.h>

#define YAKSURI_KERNEL_NULL  NULL

#define YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail)            \
    do {                                                                \
        if (cerr != cudaSuccess) {                                      \
            fprintf(stderr, "CUDA Error (%s:%s,%d): %s\n", __func__, __FILE__, __LINE__, cudaGetErrorString(cerr)); \
            rc = YAKSA_ERR__INTERNAL;                                   \
            goto fn_fail;                                               \
        }                                                               \
    } while (0)

typedef struct yaksuri_cudai_type_s {
    void (*pack) (const void *inbuf, void *outbuf, uintptr_t count, yaksa_op_t op,
                  yaksuri_cudai_md_s * md, int n_threads, int n_blocks_x, int n_blocks_y,
                  int n_blocks_z, cudaStream_t stream);
    void (*unpack) (const void *inbuf, void *outbuf, uintptr_t count, yaksa_op_t op,
                    yaksuri_cudai_md_s * md, int n_threads, int n_blocks_x, int n_blocks_y,
                    int n_blocks_z, cudaStream_t stream);
    const char *name;
    yaksuri_cudai_md_s *md;
    pthread_mutex_t mdmutex;
    uintptr_t num_elements;
} yaksuri_cudai_type_s;

#define YAKSURI_CUDAI_INFO__DEFAULT_IOV_PUP_THRESHOLD   (16384)

typedef struct {
    uintptr_t iov_pack_threshold;
    uintptr_t iov_unpack_threshold;
    struct {
        bool is_valid;
        struct cudaPointerAttributes attr;
    } inbuf, outbuf;
} yaksuri_cudai_info_s;

typedef struct {
    cudaEvent_t event;
} yaksuri_cudai_event_s;

int yaksuri_cudai_finalize_hook(void);
int yaksuri_cudai_type_create_hook(yaksi_type_s * type);
int yaksuri_cudai_type_free_hook(yaksi_type_s * type);
int yaksuri_cudai_info_create_hook(yaksi_info_s * info);
int yaksuri_cudai_info_free_hook(yaksi_info_s * info);
int yaksuri_cudai_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                                     unsigned int vallen);

int yaksuri_cudai_event_record(int device, void **event);
int yaksuri_cudai_event_query(void *event, int *completed);
int yaksuri_cudai_add_dependency(int device1, int device2);

int yaksuri_cudai_get_ptr_attr(const void *inbuf, void *outbuf, yaksi_info_s * info,
                               yaksur_ptr_attr_s * inattr, yaksur_ptr_attr_s * outattr);

int yaksuri_cudai_md_alloc(yaksi_type_s * type);
int yaksuri_cudai_populate_pupfns(yaksi_type_s * type);

int yaksuri_cudai_ipack_with_stream(const void *inbuf, void *outbuf, uintptr_t count,
                                    yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                    int target, void *stream);
int yaksuri_cudai_iunpack_with_stream(const void *inbuf, void *outbuf, uintptr_t count,
                                      yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                      int target, void *stream);
int yaksuri_cudai_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                        yaksi_info_s * info, yaksa_op_t op, int target);
int yaksuri_cudai_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                          yaksi_info_s * info, yaksa_op_t op, int target);
int yaksuri_cudai_synchronize(int target);
int yaksuri_cudai_launch_hostfn(void *stream, yaksur_hostfn_t fn, void *userData);
int yaksuri_cudai_flush_all(void);
int yaksuri_cudai_pup_is_supported(yaksi_type_s * type, yaksa_op_t op, bool * is_supported);
uintptr_t yaksuri_cudai_get_iov_pack_threshold(yaksi_info_s * info);
uintptr_t yaksuri_cudai_get_iov_unpack_threshold(yaksi_info_s * info);

static inline cudaStream_t *yaksuri_cudai_get_stream(int device)
{
    if (!yaksuri_cudai_global.streams[device].created) {
        int cur_device;
        cudaGetDevice(&cur_device);

        cudaSetDevice(device);
        cudaStreamCreateWithFlags(&yaksuri_cudai_global.streams[device].stream,
                                  cudaStreamNonBlocking);
        yaksuri_cudai_global.streams[device].created = true;

        cudaSetDevice(cur_device);
    }
    return &yaksuri_cudai_global.streams[device].stream;
}

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* YAKSURI_CUDAI_H_INCLUDED */
