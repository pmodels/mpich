/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef YAKSU_BASE_H_INCLUDED
#define YAKSU_BASE_H_INCLUDED

#define YAKSU_MAX(x, y)  ((x) > (y) ? (x) : (y))
#define YAKSU_MIN(x, y)  ((x) < (y) ? (x) : (y))
#define YAKSU_CEIL(x, y) (((x) / (y)) + !!((x) % (y)))

#define YAKSU_ERR_CHKANDJUMP(check, rc, errcode, label) \
    do {                                                \
        if ((check)) {                                  \
            (rc) = (errcode);                           \
            goto label;                                 \
        }                                               \
    } while (0)

#define YAKSU_ERR_CHECK(rc, label)              \
    do {                                        \
        if (rc != YAKSA_SUCCESS)                \
            goto label;                         \
    } while (0)

#endif /* YAKSU_BASE_H_INCLUDED */
