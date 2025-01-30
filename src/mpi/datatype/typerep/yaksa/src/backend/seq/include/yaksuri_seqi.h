/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_SEQI_H_INCLUDED
#define YAKSURI_SEQI_H_INCLUDED

#include "yaksi.h"

#define YAKSURI_KERNEL_NULL   NULL

typedef struct yaksuri_seqi_type_s {
    int (*pack) (const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                 yaksa_op_t op);
    int (*unpack) (const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                   yaksa_op_t op);
    const char *name;
} yaksuri_seqi_type_s;

#define YAKSURI_SEQI_INFO__DEFAULT_IOV_PUP_THRESHOLD   (16384)

typedef struct {
    uintptr_t iov_pack_threshold;
    uintptr_t iov_unpack_threshold;
} yaksuri_seqi_info_s;

int yaksuri_seqi_populate_pupfns(yaksi_type_s * type);

#endif /* YAKSURI_SEQI_H_INCLUDED */
