/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef YAKSURI_ZE_PRE_H_INCLUDED
#define YAKSURI_ZE_PRE_H_INCLUDED

/* This is a API header for the ze device and should not include any
 * internal headers, except for yaksa_config.h, in order to get the
 * configure checks. */

typedef struct {
    void *priv;
} yaksuri_ze_type_s;

typedef struct {
    void *priv;
} yaksuri_ze_info_s;

#endif /* YAKSURI_ZE_PRE_H_INCLUDED */
