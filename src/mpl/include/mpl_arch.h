/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_ARCH_H_INCLUDED
#define MPL_ARCH_H_INCLUDED

#include <stdint.h>

typedef enum {
    MPL_ARCH_FLAG__UNKNOWN = 0,
    MPL_ARCH_FLAG__AVX = 1ull << 1,
    MPL_ARCH_FLAG__AVX2 = 1ull << 2,
    MPL_ARCH_FLAG__AVX512F = 1ull << 3,
} MPL_arch_flag_t;

#define MPL_ARCH_HAS_AVX2 (mpl_arch_features & MPL_ARCH_FLAG__AVX2)
#define MPL_ARCH_HAS_AVX512F (mpl_arch_features & MPL_ARCH_FLAG__AVX512F)

extern uint64_t mpl_arch_features;
void MPL_check_arch_features(void);

#endif /* MPL_ARCH_H_INCLUDED */
