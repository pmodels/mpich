/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_HIPI_H_INCLUDED
#define YAKSURI_HIPI_H_INCLUDED

#include "yaksi.h"
#include <stdint.h>
#include <pthread.h>
#include <hip/hip_runtime_api.h>

#define HIP_P2P_ENABLED  (1)
#define HIP_P2P_DISABLED (2)
#define HIP_P2P_CLIQUES  (3)

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#include <yaksuri_hipi_base.h>

#define YAKSURI_KERNEL_NULL  NULL

#define YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail)            \
    do {                                                                \
        if (cerr != hipSuccess) {                                      \
            fprintf(stderr, "HIP Error (%s:%s,%d): %s\n", __func__, __FILE__, __LINE__, hipGetErrorString(cerr)); \
            rc = YAKSA_ERR__INTERNAL;                                   \
            goto fn_fail;                                               \
        }                                                               \
    } while (0)

typedef struct yaksuri_hipi_type_s {
    void (*pack) (const void *inbuf, void *outbuf, uintptr_t count, yaksa_op_t op,
                  yaksuri_hipi_md_s * md, int n_threads, int n_blocks_x, int n_blocks_y,
                  int n_blocks_z, hipStream_t stream);
    void (*unpack) (const void *inbuf, void *outbuf, uintptr_t count, yaksa_op_t op,
                    yaksuri_hipi_md_s * md, int n_threads, int n_blocks_x, int n_blocks_y,
                    int n_blocks_z, hipStream_t stream);
    const char *name;
    yaksuri_hipi_md_s *md;
    pthread_mutex_t mdmutex;
    uintptr_t num_elements;
} yaksuri_hipi_type_s;

#define YAKSURI_HIPI_INFO__DEFAULT_IOV_PUP_THRESHOLD   (16384)

typedef struct {
    uintptr_t iov_pack_threshold;
    uintptr_t iov_unpack_threshold;
    struct {
        bool is_valid;
        struct hipPointerAttribute_t attr;
    } inbuf, outbuf;
} yaksuri_hipi_info_s;

typedef struct {
    hipEvent_t event;
} yaksuri_hipi_event_s;

int yaksuri_hipi_finalize_hook(void);
int yaksuri_hipi_type_create_hook(yaksi_type_s * type);
int yaksuri_hipi_type_free_hook(yaksi_type_s * type);
int yaksuri_hipi_info_create_hook(yaksi_info_s * info);
int yaksuri_hipi_info_free_hook(yaksi_info_s * info);
int yaksuri_hipi_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                                    unsigned int vallen);

int yaksuri_hipi_event_record(int device, void **event);
int yaksuri_hipi_event_query(void *event, int *completed);
int yaksuri_hipi_add_dependency(int device1, int device2);

int yaksuri_hipi_get_ptr_attr(const void *inbuf, void *outbuf, yaksi_info_s * info,
                              yaksur_ptr_attr_s * inattr, yaksur_ptr_attr_s * outattr);

int yaksuri_hipi_md_alloc(yaksi_type_s * type);
int yaksuri_hipi_populate_pupfns(yaksi_type_s * type);

int yaksuri_hipi_ipack_with_stream(const void *inbuf, void *outbuf, uintptr_t count,
                                   yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                   int target, void *stream);
int yaksuri_hipi_iunpack_with_stream(const void *inbuf, void *outbuf, uintptr_t count,
                                     yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                     int target, void *stream);
int yaksuri_hipi_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                       yaksi_info_s * info, yaksa_op_t op, int target);
int yaksuri_hipi_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                         yaksi_info_s * info, yaksa_op_t op, int target);
int yaksuri_hipi_synchronize(int target);
int yaksuri_hipi_launch_hostfn(void *stream, yaksur_hostfn_t fn, void *userData);
int yaksuri_hipi_flush_all(void);
int yaksuri_hipi_pup_is_supported(yaksi_type_s * type, yaksa_op_t op, bool * is_supported);
uintptr_t yaksuri_hipi_get_iov_pack_threshold(yaksi_info_s * info);
uintptr_t yaksuri_hipi_get_iov_unpack_threshold(yaksi_info_s * info);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* YAKSURI_HIPI_H_INCLUDED */
