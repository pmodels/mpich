/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_GCC_ARM_H_INCLUDED
#define OPA_GCC_ARM_H_INCLUDED

/* let's set 8 byte boundary  for now */
typedef struct { volatile int v    OPA_ATTRIBUTE((aligned (8))); } OPA_int_t;
typedef struct { void * volatile v OPA_ATTRIBUTE((aligned (8))); } OPA_ptr_t;

#define OPA_INT_T_INITIALIZER(val_) { (val_) }
#define OPA_PTR_T_INITIALIZER(val_) { (val_) }

/* Aligned loads and stores are atomic. */
static _opa_inline int OPA_load_int(_opa_const OPA_int_t *ptr)
{
    return ptr->v;
}

/* Aligned loads and stores are atomic. */
static _opa_inline void OPA_store_int(OPA_int_t *ptr, int val)
{
    ptr->v = val;
}

/* Aligned loads and stores are atomic. */
static _opa_inline void *OPA_load_ptr(_opa_const OPA_ptr_t *ptr)
{
    return ptr->v;
}

/* Aligned loads and stores are atomic. */
static _opa_inline void OPA_store_ptr(OPA_ptr_t *ptr, void *val)
{
    ptr->v = val;
}

/*
  
 */
/* helper macros */
/* NOTE: only support ARM v7 */
#define OPA_arm_dmb_() __asm__ __volatile__  ( "dmb" ::: "memory" )
#define OPA_arm_dsb_() __asm__ __volatile__  ( "dsb" ::: "memory" )

#define OPA_write_barrier()      OPA_arm_dsb_()
#define OPA_read_barrier()       OPA_arm_dmb_()
#define OPA_read_write_barrier() OPA_arm_dsb_()
#define OPA_compiler_barrier()   __asm__ __volatile__  ( "" ::: "memory" )


static _opa_inline int   OPA_load_acquire_int(_opa_const OPA_int_t *ptr)
{
    int tmp;
    tmp = ptr->v;
    OPA_arm_dsb_();
    return tmp;
}
static _opa_inline void  OPA_store_release_int(OPA_int_t *ptr, int val)
{
    OPA_arm_dsb_();
    ptr->v = val;
}
static _opa_inline void *OPA_load_acquire_ptr(_opa_const OPA_ptr_t *ptr)
{
    void *tmp;
    tmp = ptr->v;
    OPA_arm_dsb_();
    return tmp;
}
static _opa_inline void  OPA_store_release_ptr(OPA_ptr_t *ptr, void *val)
{
    OPA_arm_dsb_();
    ptr->v = val;
}


/*
   load-link/store-conditional (LL/SC) primitives.  We LL/SC implement
   these here, which are arch-specific, then use the generic
   implementations from opa_emulated.h */

static _opa_inline int OPA_LL_int(OPA_int_t *ptr)
{
    int val;
    __asm__ __volatile__ ("ldrex %0,[%1]"
                          : "=&r" (val)
                          : "r" (&ptr->v)
                          : "cc");

    return val;
}

/* Returns non-zero if the store was successful, zero otherwise. */
static _opa_inline int OPA_SC_int(OPA_int_t *ptr, int val)
{
    int ret; /* will be overwritten by strex */
    /*
      strex returns 0 on success
     */
    __asm__ __volatile__ ("strex %0, %1, [%2]\n"
                          : "=&r" (ret)
                          : "r" (val), "r"(&ptr->v)
                          : "cc", "memory");

    return !ret;
}


/* Pointer versions of LL/SC. */

#if OPA_SIZEOF_VOID_P == 4
#define OPA_SS ""
#elif OPA_SIZEOF_VOID_P == 8
#define OPA_SS "d"
#else
#error OPA_SIZEOF_VOID_P is not 4 or 8
#endif


static _opa_inline void *OPA_LL_ptr(OPA_ptr_t *ptr)
{
    void *val;

    __asm__ __volatile__ ("ldrex"OPA_SS" %0,[%1]\n"
                          : "=&r" (val)
                          : "r" (&ptr->v)
                          : "cc");
    return val;
}

/* Returns non-zero if the store was successful, zero otherwise. */
static _opa_inline int OPA_SC_ptr(OPA_ptr_t *ptr, void *val)
{
    int ret; /* will be overwritten by strex */
    __asm__ __volatile__ ("strex"OPA_SS" %0, %1, [%2]\n"
                          : "=&r" (ret)
                          : "r" (val), "r"(&ptr->v)
                          : "cc", "memory");

    return !ret;
}





#undef OPA_SS

/* necessary to enable LL/SC emulation support */
#define OPA_LL_SC_SUPPORTED 1

/* Implement all function using LL/SC */
#define OPA_add_int_by_llsc            OPA_add_int
#define OPA_incr_int_by_llsc           OPA_incr_int
#define OPA_decr_int_by_llsc           OPA_decr_int
#define OPA_decr_and_test_int_by_llsc  OPA_decr_and_test_int
#define OPA_fetch_and_add_int_by_llsc  OPA_fetch_and_add_int
#define OPA_fetch_and_decr_int_by_llsc OPA_fetch_and_decr_int
#define OPA_fetch_and_incr_int_by_llsc OPA_fetch_and_incr_int
#define OPA_cas_ptr_by_llsc        OPA_cas_ptr
#define OPA_cas_int_by_llsc        OPA_cas_int
#define OPA_swap_ptr_by_llsc       OPA_swap_ptr
#define OPA_swap_int_by_llsc       OPA_swap_int


#include "opa_emulated.h"

#endif /* OPA_GCC_ARM_H_INCLUDED */
