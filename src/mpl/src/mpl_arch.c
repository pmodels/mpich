/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include "mpl_base.h"
#include <assert.h>

uint64_t mpl_arch_features = 0;

void MPL_Check_arch_features()
{
#if defined(MPL_FORCE_AVX512F)
    mpl_arch_features |= MPL_ARCH_FLAG__AVX512F;
#elif defined(MPL_FORCE_AVX)
    mpl_arch_features |= MPL_ARCH_FLAG__AVX;
#else

#ifdef MPL_HAVE_BUILTIN_CPU_SUPPORTS
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx512f")) {
        mpl_arch_features |= MPL_ARCH_FLAG__AVX512F;
    }
    if (__builtin_cpu_supports("avx")) {
        mpl_arch_features |= MPL_ARCH_FLAG__AVX;
    }
#else
    mpl_arch_features = 0;
#endif

#endif
}
