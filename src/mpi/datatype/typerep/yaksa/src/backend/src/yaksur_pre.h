/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSUR_PRE_H_INCLUDED
#define YAKSUR_PRE_H_INCLUDED

/* This is a API header exposed by the backend glue layer.  It should
 * not include any internal headers except: (1) yaksa_config.h, in
 * order to get the configure checks; and (2) API headers for the
 * devices (e.g., yaksuri_seq.h) */
#include <stdint.h>
#include "yaksuri_seq_pre.h"
#include "yaksuri_cuda_pre.h"
#include "yaksuri_ze_pre.h"
#include "yaksuri_hip_pre.h"

typedef struct {
    enum {
        YAKSUR_PTR_TYPE__UNREGISTERED_HOST,
        YAKSUR_PTR_TYPE__REGISTERED_HOST,
        YAKSUR_PTR_TYPE__GPU,
        YAKSUR_PTR_TYPE__MANAGED,
    } type;
    int device;
} yaksur_ptr_attr_s;

struct yaksi_type_s;
struct yaksi_info_s;

typedef struct yaksur_type_s {
    yaksuri_seq_type_s seq;
    yaksuri_cuda_type_s cuda;
    yaksuri_ze_type_s ze;
    yaksuri_hip_type_s hip;
} yaksur_type_s;

typedef struct {
    yaksur_ptr_attr_s inattr;
    yaksur_ptr_attr_s outattr;
    void *priv;
} yaksur_request_s;

typedef struct {
    bool pre_init;              /* set to true for info created before yaksa_init */
    yaksuri_seq_info_s seq;
    yaksuri_cuda_info_s cuda;
    yaksuri_ze_info_s ze;
    yaksuri_hip_info_s hip;
    void *priv;
} yaksur_info_s;

typedef void (*yaksur_hostfn_t) (void *userData);

typedef struct yaksur_gpudriver_hooks_s {
    /* miscellaneous */
    int (*get_num_devices) (int *ndevices);
    /* *INDENT-OFF* */
    bool (*check_p2p_comm) (int sdev, int ddev);
    /* *INDENT-ON* */
    int (*finalize) (void);

    /* pup functions */
    /* *INDENT-OFF* */
    uintptr_t (*get_iov_pack_threshold) (struct yaksi_info_s * info);
    uintptr_t (*get_iov_unpack_threshold) (struct yaksi_info_s * info);
    /* *INDENT-ON* */
    int (*ipack) (const void *inbuf, void *outbuf, uintptr_t count,
                  struct yaksi_type_s * type, struct yaksi_info_s * info, yaksa_op_t op,
                  int device);
    int (*iunpack) (const void *inbuf, void *outbuf, uintptr_t count, struct yaksi_type_s * type,
                    struct yaksi_info_s * info, yaksa_op_t op, int device);
    int (*pack_with_stream) (const void *inbuf, void *outbuf, uintptr_t count,
                             struct yaksi_type_s * type, struct yaksi_info_s * info, yaksa_op_t op,
                             int device, void *stream);
    int (*unpack_with_stream) (const void *inbuf, void *outbuf, uintptr_t count,
                               struct yaksi_type_s * type, struct yaksi_info_s * info,
                               yaksa_op_t op, int device, void *stream);
    int (*synchronize) (int device);    /* complete all outstanding tasks that are created by yaksa on the device */
    int (*flush_all) (void);
    int (*pup_is_supported) (struct yaksi_type_s * type, yaksa_op_t op, bool * is_supported);
    int (*launch_hostfn) (void *stream, yaksur_hostfn_t fn, void *userData);

    /* memory management */
    void *(*host_malloc) (uintptr_t size);
    void (*host_free) (void *ptr);
    void *(*gpu_malloc) (uintptr_t size, int device);
    void (*gpu_free) (void *ptr);
    int (*get_ptr_attr) (const void *inbuf, void *outbuf, struct yaksi_info_s * info,
                         yaksur_ptr_attr_s * inattr, yaksur_ptr_attr_s * outattr);

    /* events */
    int (*event_record) (int device, void **event);
    int (*event_query) (void *event, int *completed);
    int (*add_dependency) (int device1, int device2);

    /* types */
    int (*type_create) (struct yaksi_type_s * type);
    int (*type_free) (struct yaksi_type_s * type);

    /* info */
    int (*info_create) (struct yaksi_info_s * info);
    int (*info_free) (struct yaksi_info_s * info);
    int (*info_keyval_append) (struct yaksi_info_s * info, const char *key, const void *val,
                               unsigned int vallen);
} yaksur_gpudriver_hooks_s;

#endif /* YAKSUR_PRE_H_INCLUDED */
