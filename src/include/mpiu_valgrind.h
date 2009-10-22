/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* valgrind interaction macros and header include logic

   These macros are intended to simplify client request interactions with
   valgrind.  An MPICH2 source file that needs valgrind client requests if they
   are available can include this file instead and use MPIU_VG_ macros without
   worrying about whether or not valgrind is actually present, which headers to
   include, and how to include them.

   For more complicated logic this header will also define/undefine the
   preprocessor token "MPIU_VG_AVAILABLE".
 */
#ifndef MPICH_VALGRIND_H_INCLUDED
#define MPICH_VALGRIND_H_INCLUDED

#include "mpichconf.h"

#undef MPIU_VG_AVAILABLE

#if defined(MPICH_DEBUG_MEMINIT) && !defined(NVALGRIND)
#  if defined(HAVE_VALGRIND_H) && defined(HAVE_MEMCHECK_H)
#    include <valgrind.h>
#    include <memcheck.h>
#    define MPIU_VG_AVAILABLE 1
#  elif defined(HAVE_VALGRIND_VALGRIND_H) && defined(HAVE_VALGRIND_MEMCHECK_H)
#    include <valgrind/valgrind.h>
#    include <valgrind/memcheck.h>
#    define MPIU_VG_AVAILABLE 1
#  endif
#endif

/* This is only a modest subset of all of the available client requests defined
   in the valgrind headers.  As MPICH2 is modified to use more of them, this
   list should be expanded appropriately. */
#if defined(MPIU_VG_AVAILABLE)
#  define MPIU_VG_MAKE_MEM_DEFINED(addr_,len_)         VALGRIND_MAKE_MEM_DEFINED((addr_),(len_))
#  define MPIU_VG_MAKE_MEM_NOACCESS(addr_,len_)        VALGRIND_MAKE_MEM_NOACCESS((addr_),(len_))
#  define MPIU_VG_MAKE_MEM_UNDEFINED(addr_,len_)       VALGRIND_MAKE_MEM_UNDEFINED((addr_),(len_))
#  define MPIU_VG_CHECK_MEM_IS_DEFINED(addr_,len_)     VALGRIND_CHECK_MEM_IS_DEFINED((addr_),(len_))
#  define MPIU_VG_CHECK_MEM_IS_ADDRESSABLE(addr_,len_) VALGRIND_CHECK_MEM_IS_ADDRESSABLE((addr_),(len_))
#  define MPIU_VG_CREATE_BLOCK(addr_,len_,desc_)       VALGRIND_CREATE_BLOCK((addr_),(len_),(desc_))
#  define MPIU_VG_RUNNING_ON_VALGRIND()                RUNNING_ON_VALGRIND
#  define MPIU_VG_PRINTF_BACKTRACE                     VALGRIND_PRINTF_BACKTRACE

/* custom allocator client requests, you probably shouldn't use these unless you
 * really know what you are doing */
#  define MPIU_VG_CREATE_MEMPOOL(pool, rzB, is_zeroed) VALGRIND_CREATE_MEMPOOL((pool), (rzB), (is_zeroed))
#  define MPIU_VG_DESTROY_MEMPOOL(pool)                VALGRIND_DESTROY_MEMPOOL((pool))
#  define MPIU_VG_MEMPOOL_ALLOC(pool, addr, size)      VALGRIND_MEMPOOL_ALLOC((pool), (addr), (size))
#  define MPIU_VG_MEMPOOL_FREE(pool, addr)             VALGRIND_MEMPOOL_FREE((pool), (addr))

#else /* !defined(MPIU_VG_AVAILABLE) */
#  define MPIU_VG_MAKE_MEM_DEFINED(addr_,len_)         do{}while(0)
#  define MPIU_VG_MAKE_MEM_NOACCESS(addr_,len_)        do{}while(0)
#  define MPIU_VG_MAKE_MEM_UNDEFINED(addr_,len_)       do{}while(0)
#  define MPIU_VG_CHECK_MEM_IS_DEFINED(addr_,len_)     do{}while(0)
#  define MPIU_VG_CHECK_MEM_IS_ADDRESSABLE(addr_,len_) do{}while(0)
#  define MPIU_VG_CREATE_BLOCK(addr_,len_,desc_)       do{}while(0)
#  define MPIU_VG_RUNNING_ON_VALGRIND()                (0)/*always false*/
#  if defined(HAVE_MACRO_VA_ARGS)
#    define MPIU_VG_PRINTF_BACKTRACE(...)              do{}while(0)
#  else
#    define MPIU_VG_PRINTF_BACKTRACE MPIU_VG_printf_do_nothing_func
static inline void MPIU_VG_printf_do_nothing_func(char *fmt, ...)
{
    /* do nothing */
}
#  endif /* defined(HAVE_MACRO_VA_ARGS) */
#  define MPIU_VG_CREATE_MEMPOOL(pool, rzB, is_zeroed) do{}while(0)
#  define MPIU_VG_DESTROY_MEMPOOL(pool)                do{}while(0)
#  define MPIU_VG_MEMPOOL_ALLOC(pool, addr, size)      do{}while(0)
#  define MPIU_VG_MEMPOOL_FREE(pool, addr)             do{}while(0)

#endif /* defined(MPIU_VG_AVAILABLE) */


#endif /* !defined(MPICH_VALGRIND_H_INCLUDED) */

