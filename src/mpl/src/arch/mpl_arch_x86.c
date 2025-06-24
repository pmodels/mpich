/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include "mpl_base.h"

uint64_t mpl_arch_features = MPL_ARCH_FLAG__UNKNOWN;

#ifndef MPL_HAVE_BUILTIN_CPU_SUPPORTS
static void x86_detect_cpu_features(void);

#define X86_CPUID_GET_MODEL      0x00000001u
#define X86_CPUID_GET_BASE_VALUE 0x00000000u
#define X86_CPUID_GET_EXTD_VALUE 0x00000007u

#define x86_cpuid(level, a, b, c, d) \
    __asm__ volatile ("cpuid" \
                  : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) \
                  : "0"(level))

#define x86_xgetbv(index, eax, edx) \
    __asm__ volatile (".byte 0x0f, 0x01, 0xd0" \
                  : "=a"(eax), "=d"(edx) : "c" (index))

void x86_detect_cpu_features(void)
{
    if (mpl_arch_features == MPL_ARCH_FLAG__UNKNOWN) {
        uint32_t base_value;
        uint32_t eax, ebx, ecx, edx;

        x86_cpuid(X86_CPUID_GET_BASE_VALUE, &eax, &ebx, &ecx, &edx);
        base_value = eax;

        if (base_value >= 1) {
            x86_cpuid(X86_CPUID_GET_MODEL, &eax, &ebx, &ecx, &edx);
            if ((ecx & 0x18000000) == 0x18000000) {
                x86_xgetbv(0, eax, edx);
                if ((eax & 0x6) == 0x6) {
                    mpl_arch_features |= MPL_ARCH_FLAG__AVX;
                }
            }
        }
        if (base_value >= 7) {
            x86_cpuid(X86_CPUID_GET_EXTD_VALUE, &eax, &ebx, &ecx, &edx);
            if ((mpl_arch_features & MPL_ARCH_FLAG__AVX) && (ebx & (1 << 5))) {
                mpl_arch_features |= MPL_ARCH_FLAG__AVX2;
            }
            if (ebx & (1 << 16)) {
                mpl_arch_features |= MPL_ARCH_FLAG__AVX512F;
            }
        }
    }
}
#endif

void MPL_check_arch_features(void)
{
#if defined(MPL_FORCE_AVX512F)
    mpl_arch_features |= MPL_ARCH_FLAG__AVX512F;
#elif defined(MPL_FORCE_AVX)
    mpl_arch_features |= MPL_ARCH_FLAG__AVX;
#else

    mpl_arch_features = 0;
#ifdef MPL_HAVE_BUILTIN_CPU_SUPPORTS
    __builtin_cpu_init();
#ifdef MPL_HAVE_BUILTIN_CPU_SUPPORTS_AVX512F
    if (__builtin_cpu_supports("avx512f")) {
        mpl_arch_features |= MPL_ARCH_FLAG__AVX512F;
    }
#endif
    if (__builtin_cpu_supports("avx")) {
        mpl_arch_features |= MPL_ARCH_FLAG__AVX;
    }
#else
    x86_detect_cpu_features();
#endif

#endif
}
