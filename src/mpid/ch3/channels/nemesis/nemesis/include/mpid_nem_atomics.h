/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_ATOMICS_H
#define MPID_NEM_ATOMICS_H

#include <mpichconf.h>
#include <mpidi_ch3i_nemesis_conf.h>

static inline void *MPID_NEM_SWAP (volatile void *ptr, void *val)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    __asm__ __volatile__ ("xchgl %0,%1"
                          :"=r" (val), "=m" (*(void **)ptr)
                          : "0" (val),  "m" (*(void **)ptr));
    return val;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    __asm__ __volatile__ ("xchgq %0,%1"
                          :"=r" (val), "=m" (*(void **)ptr)
                          : "0" (val),  "m" (*(void **)ptr));
    return val;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (*(void **)val)
                          : "r" (ptr), "0" (val));
    return val;
#else
#error No swap function defined for this architecture
#endif
}


static inline void *MPID_NEM_CAS (volatile void *ptr, void *oldv, void *newv)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    void *prev;
    __asm__ __volatile__ ("lock ; cmpxchgl %2,%3"
                          : "=a" (prev), "=m" (*(void **)ptr)
                          : "q" (newv), "m" (*(void **)ptr), "0" (oldv));
    return prev;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    void *prev;
    __asm__ __volatile__ ("lock ; cmpxchgq %2,%3"
                          : "=a" (prev), "=m" (*(void **)ptr)
                          : "q" (newv), "m" (*(void **)ptr), "0" (oldv));
    return prev;   
#elif defined(HAVE_GCC_AND_IA64_ASM)
    void *prev;
    __asm__ __volatile__ ("mov ar.ccv=%2;;"
                          "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                          : "=r"(prev), "=m"(*(void **)ptr)
                          : "r"(oldv), "r"(ptr), "r"(newv));
    return prev;   
#else
#error No compare-and-swap function defined for this architecture
#endif
}

static inline int MPID_NEM_CAS_INT (volatile int *ptr, int oldv, int newv)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    int prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv));
    return prev;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    int prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv));
    return prev;   
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int prev;

    switch (sizeof(int)) /* this switch statement should be optimized out */
    {
    case 8:
        __asm__ __volatile__ ("mov ar.ccv=%2;;"
                              "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                              : "=r"(prev), "=m"(*ptr)
                              : "r"(oldv), "r"(ptr), "r"(newv));
        break;
    case 4:
        __asm__ __volatile__ ("zxt4 %2=%2;;" /* don't want oldv sign-extended to 64 bits */
			      "mov ar.ccv=%2;;"
			      "cmpxchg4.rel %0=[%3],%4,ar.ccv"
                              : "=r"(prev), "=m"(*ptr)
			      : "r"(oldv), "r"(ptr), "r"(newv));
        break;
    default:
        MPIU_Assertp (0);
    }
    
    return prev;   
#else
#error No compare-and-swap function defined for this architecture
#endif
}

static inline int MPID_NEM_FETCH_AND_ADD (volatile int *ptr, int val)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    __asm__ __volatile__ ("lock ; xadd %0,%1"
                          : "=r" (val), "=m" (*ptr)
                          :  "0" (val),  "m" (*ptr));
    return val;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int new;
    int prev;
    int old;
    
    do
    {
        old = *ptr;
        new = old + val;
        __asm__ __volatile__ ("mov ar.ccv=%1;;"
                              "cmpxchg4.rel %0=[%3],%4,ar.ccv"
                              : "=r"(prev), "=m"(*ptr)
                              : "rO"(old), "r"(ptr), "r"(new));
    }
    while (prev != old);
    return new;
#else
    /* default implementation */
    int new;
    int prev;
    int old;
    
    MPIU_Assert (sizeof(int) == sizeof(void *));
    do
    {
        old = *ptr;
        new = old + val;
        prev = MPID_NEM_CAS_INT(ptr, old, new);
    }
    while (prev != old);
    return new;
#endif
}

static inline void MPID_NEM_ATOMIC_ADD (int *ptr, int val)
{
#if  defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    __asm__ __volatile__ ("lock ; add %1,%0"
                          :"=m" (*ptr)
                          :"ir" (val), "m" (*ptr));
    return;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    MPIU_Assertp (0);; /* implement atomic add */
#else
#error No fetch-and-add function defined for this architecture
#endif
}

static inline int MPID_NEM_FETCH_AND_INC (volatile int *ptr)
{
#ifdef HAVE_GCC_AND_IA64_ASM
    int val;
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"
                          : "=r"(val), "=m" (*ptr)
                          : "r"(ptr), "i" (1));
    return val;
#else
    /* default implementation */
    return MPID_NEM_FETCH_AND_ADD (ptr, 1);
#endif
}

static inline int MPID_NEM_FETCH_AND_DEC (volatile int *ptr)
{
#ifdef HAVE_GCC_AND_IA64_ASM
    int val;
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"
                          : "=r"(val), "=m"(*ptr)
                          : "r"(ptr), "i" (-1));
    return val;
#else
    /* default implementation */
    return MPID_NEM_FETCH_AND_ADD (ptr, -1);
#endif
}

static inline void MPID_NEM_ATOMIC_INC (volatile int *ptr)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; incl %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; incq %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return;
    
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int val;
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"
                          : "=r"(val), "=m"(*ptr)
                          : "r"(ptr), "i"(1));
    return;
#else
#error No fetch-and-add function defined for this architecture
#endif
}

static inline void MPID_NEM_ATOMIC_DEC (volatile int *ptr)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; decl %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; decq %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int val;
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"
                          : "=r"(val), "=m"(*ptr)
                          : "r"(ptr), "i"(-1));
    return;
#else
#error No fetch-and-add function defined for this architecture
#endif
}


#ifdef HAVE_GCC_AND_PENTIUM_ASM

#ifdef HAVE_GCC_ASM_AND_X86_SFENCE
#define MPID_NEM_WRITE_BARRIER() __asm__ __volatile__  ( "sfence" ::: "memory" )
#else /* HAVE_GCC_ASM_AND_X86_SFENCE */
#define MPID_NEM_WRITE_BARRIER()
#endif /* HAVE_GCC_ASM_AND_X86_SFENCE */

#ifdef HAVE_GCC_ASM_AND_X86_LFENCE
/*
  #define MPID_NEM_READ_BARRIER() __asm__ __volatile__  ( ".byte 0x0f, 0xae, 0xe8" ::: "memory" ) */
#define MPID_NEM_READ_BARRIER() __asm__ __volatile__  ( "lfence" ::: "memory" )
#else /* HAVE_GCC_ASM_AND_X86_LFENCE */
#define MPID_NEM_READ_BARRIER()
#endif /* HAVE_GCC_ASM_AND_X86_LFENCE */


#ifdef HAVE_GCC_ASM_AND_X86_MFENCE
/*
  #define MPID_NEM_READ_WRITE_BARRIER() __asm__ __volatile__  ( ".byte 0x0f, 0xae, 0xf0" ::: "memory" )
*/
#define MPID_NEM_READ_WRITE_BARRIER() __asm__ __volatile__  ( "mfence" ::: "memory" )
#else /* HAVE_GCC_ASM_AND_X86_MFENCE */
#define MPID_NEM_READ_WRITE_BARRIER()
#endif /* HAVE_GCC_ASM_AND_X86_MFENCE */

#elif defined(HAVE_MASM_AND_X86)
#define MPID_NEM_WRITE_BARRIER()
#define MPID_NEM_READ_BARRIER() __asm { __asm _emit 0x0f __asm _emit 0xae __asm _emit 0xe8 }
#define MPID_NEM_READ_WRITE_BARRIER()

#elif defined(HAVE_GCC_AND_IA64_ASM)
#define MPID_NEM_WRITE_BARRIER() __asm__ __volatile__  ("mf" ::: "memory" )
#define MPID_NEM_READ_BARRIER() __asm__ __volatile__  ("mf" ::: "memory" )
#define MPID_NEM_READ_WRITE_BARRIER() __asm__ __volatile__  ("mf" ::: "memory" )

#else
#define MPID_NEM_WRITE_BARRIER()
#define MPID_NEM_READ_BARRIER()
#define MPID_NEM_READ_WRITE_BARRIER()
#endif /* HAVE_GCC_AND_PENTIUM_ASM */


#endif /* MPID_NEM_ATOMICS_H */
