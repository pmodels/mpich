/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME needs to be converted to the new OPA_int_t-style of functions */

#ifndef OPA_GCC_IA64_H_INCLUDED
#define OPA_GCC_IA64_H_INCLUDED

/* XXX DJG FIXME do we need to align these? */
typedef struct { volatile int v;    } OPA_int_t;
typedef struct { void * volatile v; } OPA_ptr_t;

#define OPA_INT_T_INITIALIZER(val_) { (val_) }
#define OPA_PTR_T_INITIALIZER(val_) { (val_) }

/* Aligned loads and stores are atomic on ia64. */
static _opa_inline int OPA_load_int(_opa_const OPA_int_t *ptr)
{
    return ptr->v;
}

/* Aligned loads and stores are atomic on ia64. */
static _opa_inline void OPA_store_int(OPA_int_t *ptr, int val)
{
    ptr->v = val;
}

/* Aligned loads and stores are atomic on ia64. */
static _opa_inline void *OPA_load_ptr(_opa_const OPA_ptr_t *ptr)
{
    return ptr->v;
}

/* Aligned loads and stores are atomic on ia64. */
static _opa_inline void OPA_store_ptr(OPA_ptr_t *ptr, void *val)
{
    ptr->v = val;
}

#define OPA_add_int_by_faa OPA_add_int
#define OPA_incr_int_by_faa OPA_incr_int
#define OPA_decr_int_by_faa OPA_decr_int
#define OPA_fetch_and_decr_int_by_faa OPA_fetch_and_decr_int
#define OPA_fetch_and_incr_int_by_faa OPA_fetch_and_incr_int

static _opa_inline int OPA_decr_and_test_int(OPA_int_t *ptr)
{
    int val;
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"
                          : "=r"(val), "=m"(ptr->v)
                          : "r"(&ptr->v), "i"(-1));
    return val == 1;
}

static _opa_inline int OPA_cas_int(OPA_int_t *ptr, int oldv, int newv)
{
    int prev;

#if OPA_SIZEOF_INT == 8
    __asm__ __volatile__ ("mov ar.ccv=%1;;"
                          "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                          : "=r"(prev), "=m"(ptr->v)
                          : "rO"(oldv), "r"(&ptr->v), "r"(newv)
                          : "memory");
    break;
#elif OPA_SIZEOF_INT == 4
    __asm__ __volatile__ ("zxt4 %1=%1;;" /* don't want oldv sign-extended to 64 bits */
                          "mov ar.ccv=%1;;"
                          "cmpxchg4.rel %0=[%3],%4,ar.ccv"
                          : "=r"(prev), "=m"(ptr->v)
                          : "r0"(oldv), "r"(&ptr->v), "r"(newv)
                          : "memory");
#else
#error OPA_SIZEOF_INT is not 4 or 8
#endif

    return prev;
}

/* IA64 has a fetch-and-add instruction that only accepts immediate
   values of -16, -8, -4, -1, 1, 4, 8, and 16.  So we check for these
   values before falling back to the CAS implementation. */
#define OPA_IA64_FAA_CASE_MACRO(ptr, val) case val: {   \
    int prev;                                           \
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"    \
                          : "=r"(prev), "=m"(ptr->v)    \
                          : "r"(&ptr->v), "i"(val));    \
    return prev;                                        \
    }                                                   \
    break


static _opa_inline int OPA_fetch_and_add_int(OPA_int_t *ptr, int val)
{
    switch (val)
    {
        OPA_IA64_FAA_CASE_MACRO(ptr, -16);
        OPA_IA64_FAA_CASE_MACRO(ptr,  -8);
        OPA_IA64_FAA_CASE_MACRO(ptr,  -4);
        OPA_IA64_FAA_CASE_MACRO(ptr,  -1);
        OPA_IA64_FAA_CASE_MACRO(ptr,   1);
        OPA_IA64_FAA_CASE_MACRO(ptr,   4);
        OPA_IA64_FAA_CASE_MACRO(ptr,   8);
        OPA_IA64_FAA_CASE_MACRO(ptr,  16);
    default:
        {
            int cmp;
            int prev = OPA_load_int(ptr);

            do {
                cmp = prev;
                prev = OPA_cas_int(ptr, cmp, val);
            } while (prev != cmp);

            return prev;
        }
    }
}
#undef OPA_IA64_FAA_CASE_MACRO


static _opa_inline void *OPA_cas_ptr(OPA_ptr_t *ptr, void *oldv, void *newv)
{
    void *prev;
    __asm__ __volatile__ ("mov ar.ccv=%1;;"
                          "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                          : "=r"(prev), "=m"(ptr->v)
                          : "rO"(oldv), "r"(&ptr->v), "r"(newv));
    return prev;
}

static _opa_inline void *OPA_swap_ptr(OPA_ptr_t *ptr, void *val)
{
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (ptr->v)
                          : "r" (&ptr->v), "0" (val));
    return val;
}


static _opa_inline int OPA_swap_int(OPA_int_t *ptr, int val)
{
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (ptr->v)
                          : "r" (&ptr->v), "0" (val));
    return val;
}


#define OPA_write_barrier()      __asm__ __volatile__  ("mf" ::: "memory" )
#define OPA_read_barrier()       __asm__ __volatile__  ("mf" ::: "memory" )
#define OPA_read_write_barrier() __asm__ __volatile__  ("mf" ::: "memory" )

#include "opa_emulated.h"

#endif /* OPA_GCC_IA64_H_INCLUDED */
