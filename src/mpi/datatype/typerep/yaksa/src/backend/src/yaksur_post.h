/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSUR_POST_H_INCLUDED
#define YAKSUR_POST_H_INCLUDED

#include "yaksuri_seq_post.h"
#include "yaksuri_cuda_post.h"
#include "yaksuri_ze_post.h"
#include "yaksuri_hip_post.h"

int yaksur_init_hook(yaksi_info_s * info);
int yaksur_finalize_hook(void);
int yaksur_type_create_hook(yaksi_type_s * type);
int yaksur_type_free_hook(yaksi_type_s * type);
int yaksur_request_create_hook(yaksi_request_s * request);
int yaksur_request_free_hook(yaksi_request_s * request);
int yaksur_info_create_hook(yaksi_info_s * info);
int yaksur_info_free_hook(yaksi_info_s * info);
int yaksur_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                              unsigned int vallen);

int yaksur_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                 yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request);
int yaksur_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                   yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request);
int yaksur_request_test(yaksi_request_s * request);
int yaksur_request_wait(yaksi_request_s * request);

#endif /* YAKSUR_POST_H_INCLUDED */
