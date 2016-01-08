/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_TIMER_LINUX86_CYCLE_H_INCLUDED
#define MPL_TIMER_LINUX86_CYCLE_H_INCLUDED

static inline int MPL_wtime(MPL_time_t *timeval)
{
/* The rdtsc instruction is not a "serializing" instruction, so the
   processor is free to reorder it.  In order to get more accurate
   timing numbers with rdtsc, we need to put a serializing
   instruction, like cpuid, before rdtsc.  X86_64 architectures have
   the rdtscp instruction which is synchronizing, we use this when we
   can. */
#ifdef MPL_LINUX86_CYCLE_RDTSCP
    __asm__ __volatile__("push %%rbx ; cpuid ; rdtsc ; pop %%rbx ; shl $32, %%rdx; or %%rdx, %%rax" : "=a" (timeval) : : "ecx", "rdx");

#elif defined(MPL_LINUX86_CYCLE_CPUID_RDTSC64)
/* Here we have to save the rbx register for when the compiler is
   generating position independent code (e.g., when it's generating
   shared libraries) */
    __asm__ __volatile__("push %%rbx ; cpuid ; rdtsc ; pop %%rbx" : "=A" (timeval) : : "ecx");

#elif defined(MPL_LINUX86_CYCLE_CPUID_RDTSC32)
/* Here we have to save the ebx register for when the compiler is
   generating position independent code (e.g., when it's generating
   shared libraries) */
    __asm__ __volatile__("push %%ebx ; cpuid ; rdtsc ; pop %%ebx" : "=A" (timeval) : : "ecx");

#elif defined(MPL_LINUX86_CYCLE_RDTSC)
/* The configure test using cpuid must have failed, try just rdtsc by itself */
    __asm__ __volatile__("rdtsc" : "=A" (timeval));

#else
#error Dont know which Linux timer to use
#endif

    return MPL_TIMER_SUCCESS;
}

#endif
