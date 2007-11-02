/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#define USE_GCC_X86_CYCLE_ASM 1
#define USE_WIN_X86_CYCLE_ASM 2

#if MPICH_CPU_TICK_TYPE == USE_GCC_X86_CYCLE_ASM
/* The rdtsc instruction is not a "serializing" instruction, so the
   processor is free to reorder it.  In order to get more accurate
   timing numbers with rdtsc, we need to put a serializing
   instruction, like cpuid, before rdtsc.  X86_64 architectures have
   the rdtscp instruction which is synchronizing, we use this when we
   can. */
#ifdef GCC_X86_CYCLE_RDTSCP
#define MPID_CPU_TICK(var_ptr)                                                                          \
    __asm__ __volatile__("rdtscp; shl $32, %%rdx; or %%rdx, %%rax" : "=a" (*var_ptr) : : "ecx", "rdx")
#elif defined(GCC_X86_CYCLE_CPUID_RDTSC)
/* Here we have to save the ebx register for when the compiler is
   generating position independent code (e.g., when it's generating
   shared libraries) */
#define MPID_CPU_TICK(var_ptr)                                                                     \
     __asm__ __volatile__("push %%ebx ; cpuid ; rdtsc ; pop %%ebx" : "=A" (*var_ptr) : : "ecx")
#elif defined(GCC_X86_CYCLE_RDTSC)
/* The configure test using cpuid must have failed, try just rdtsc by itself */
#define MPID_CPU_TICK(var_ptr) __asm__ __volatile__("rdtsc" : "=A" (*var_ptr))
#else
#error Dont know which Linux timer to use
#endif

typedef long long MPID_CPU_Tick_t;

#elif MPICH_CPU_TICK_TYPE == USE_WIN_X86_CYCLE_ASM
/* This cycle counter is the read time stamp (rdtsc) instruction with Microsoft asm */
#define MPID_CPU_TICK(var_ptr) \
{ \
    register int *f1 = (int*)var_ptr; \
    __asm cpuid \
    __asm rdtsc \
    __asm mov ecx, f1 \
    __asm mov [ecx], eax \
    __asm mov [ecx + TYPE int], edx \
}
typedef unsigned __int64 MPID_CPU_Tick_t;

#else
/*#error CPU tick instruction needed to count progress time*/
#undef MPID_CPU_TICK
#endif

extern volatile unsigned int MPIDI_CH3I_progress_completions;
int handle_shm_read(MPIDI_VC_t *vc, int nb);
int MPIDI_CH3I_SHM_write_progress(MPIDI_VC_t * vc);
