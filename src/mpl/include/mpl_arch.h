/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_ARCH_H_INCLUDED
#define MPL_ARCH_H_INCLUDED

#include <stdint.h>

#define MPL_ARCH_FLAG__AVX (0x1ULL)
#define MPL_ARCH_FLAG__AVX512F (0x1ULL << 1)

#define MPL_ARCH_HAS_AVX (mpl_arch_features & MPL_ARCH_FLAG__AVX)
#define MPL_ARCH_HAS_AVX512F (mpl_arch_features & MPL_ARCH_FLAG__AVX512F)

extern uint64_t mpl_arch_features;
void MPL_Check_arch_features();

#endif /* MPL_ARCH_H_INCLUDED */
