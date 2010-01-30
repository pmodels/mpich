/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* IMPORTANT!!!: you must define MPL_VG_ENABLED before including mpl.h if you
   want the the actual valgrind macros to be expanded when an MPL_VG_ macro is
   used */

/* this file should not be included directly, it expects to be included from
 * mpl.h with the results of various configure tests and several system
 * libraries already included */
#if !defined(MPL_H_INCLUDED)
#error do not include mpl_valgrind.h directly, use mpl.h instead
#endif

/* valgrind interaction macros and header include logic

   These macros are intended to simplify client request interactions with
   valgrind.  A client source file that needs valgrind client requests if they
   are available can include this file instead and use MPL_VG_ macros without
   worrying about whether or not valgrind is actually present, which headers to
   include, and how to include them.

   For more complicated logic this header will also define/undefine the
   preprocessor token "MPL_VG_AVAILABLE".

 */
#ifndef MPL_VALGRIND_H_INCLUDED
#define MPL_VALGRIND_H_INCLUDED

#undef MPL_VG_AVAILABLE

#if defined(MPL_VG_ENABLED)
#  if defined(MPICH_DEBUG_MEMINIT) && !defined(NVALGRIND)
#    if defined(MPL_HAVE_VALGRIND_H) && defined(MPL_HAVE_MEMCHECK_H)
#      include <valgrind.h>
#      include <memcheck.h>
#      define MPL_VG_AVAILABLE 1
#    elif defined(MPL_HAVE_VALGRIND_VALGRIND_H) && defined(MPL_HAVE_VALGRIND_MEMCHECK_H)
#      include <valgrind/valgrind.h>
#      include <valgrind/memcheck.h>
#      define MPL_VG_AVAILABLE 1
#    endif
#  endif
#endif

/* This is only a modest subset of all of the available client requests defined
   in the valgrind headers.  As MPICH2 is modified to use more of them, this
   list should be expanded appropriately. */
#if defined(MPL_VG_AVAILABLE)
#  if defined(VALGRIND_MAKE_MEM_DEFINED)
/* this valgrind is version 3.2.0 or later */
#  define MPL_VG_MAKE_MEM_DEFINED(addr_,len_)         VALGRIND_MAKE_MEM_DEFINED((addr_),(len_))
#  define MPL_VG_MAKE_MEM_NOACCESS(addr_,len_)        VALGRIND_MAKE_MEM_NOACCESS((addr_),(len_))
#  define MPL_VG_MAKE_MEM_UNDEFINED(addr_,len_)       VALGRIND_MAKE_MEM_UNDEFINED((addr_),(len_))
#  define MPL_VG_CHECK_MEM_IS_DEFINED(addr_,len_)     VALGRIND_CHECK_MEM_IS_DEFINED((addr_),(len_))
#  define MPL_VG_CHECK_MEM_IS_ADDRESSABLE(addr_,len_) VALGRIND_CHECK_MEM_IS_ADDRESSABLE((addr_),(len_))
#  else
/* this is an older version of valgrind.  Use the older (subtly misleading) names */
#    define MPL_VG_MAKE_MEM_DEFINED(addr_,len_)         VALGRIND_MAKE_READABLE((addr_),(len_))
#    define MPL_VG_MAKE_MEM_NOACCESS(addr_,len_)        VALGRIND_MAKE_NOACCESS((addr_),(len_))
#    define MPL_VG_MAKE_MEM_UNDEFINED(addr_,len_)       VALGRIND_MAKE_WRITABLE((addr_),(len_))
#    define MPL_VG_CHECK_MEM_IS_DEFINED(addr_,len_)     VALGRIND_CHECK_READABLE((addr_),(len_))
#    define MPL_VG_CHECK_MEM_IS_ADDRESSABLE(addr_,len_) VALGRIND_CHECK_WRITABLE((addr_),(len_))
#  endif
#  define MPL_VG_CREATE_BLOCK(addr_,len_,desc_)       VALGRIND_CREATE_BLOCK((addr_),(len_),(desc_))
#  define MPL_VG_RUNNING_ON_VALGRIND()                RUNNING_ON_VALGRIND
#  define MPL_VG_PRINTF_BACKTRACE                     VALGRIND_PRINTF_BACKTRACE

/* custom allocator client requests, you probably shouldn't use these unless you
 * really know what you are doing */
#  define MPL_VG_CREATE_MEMPOOL(pool, rzB, is_zeroed) VALGRIND_CREATE_MEMPOOL((pool), (rzB), (is_zeroed))
#  define MPL_VG_DESTROY_MEMPOOL(pool)                VALGRIND_DESTROY_MEMPOOL((pool))
#  define MPL_VG_MEMPOOL_ALLOC(pool, addr, size)      VALGRIND_MEMPOOL_ALLOC((pool), (addr), (size))
#  define MPL_VG_MEMPOOL_FREE(pool, addr)             VALGRIND_MEMPOOL_FREE((pool), (addr))

#else /* !defined(MPL_VG_AVAILABLE) */
#  define MPL_VG_MAKE_MEM_DEFINED(addr_,len_)         do {} while (0)
#  define MPL_VG_MAKE_MEM_NOACCESS(addr_,len_)        do {} while (0)
#  define MPL_VG_MAKE_MEM_UNDEFINED(addr_,len_)       do {} while (0)
#  define MPL_VG_CHECK_MEM_IS_DEFINED(addr_,len_)     do {} while (0)
#  define MPL_VG_CHECK_MEM_IS_ADDRESSABLE(addr_,len_) do {} while (0)
#  define MPL_VG_CREATE_BLOCK(addr_,len_,desc_)       do {} while (0)
#  define MPL_VG_RUNNING_ON_VALGRIND()                (0)       /*always false */
#  if defined(MPL_HAVE_MACRO_VA_ARGS)
#    define MPL_VG_PRINTF_BACKTRACE(...)              do {} while (0)
#  else
#    define MPL_VG_PRINTF_BACKTRACE MPL_VG_printf_do_nothing_func
static inline void MPL_VG_printf_do_nothing_func(char *fmt, ...)
{
    /* do nothing */
}
#  endif /* defined(MPL_HAVE_MACRO_VA_ARGS) */
#  define MPL_VG_CREATE_MEMPOOL(pool, rzB, is_zeroed) do {} while (0)
#  define MPL_VG_DESTROY_MEMPOOL(pool)                do {} while (0)
#  define MPL_VG_MEMPOOL_ALLOC(pool, addr, size)      do {} while (0)
#  define MPL_VG_MEMPOOL_FREE(pool, addr)             do {} while (0)

#endif /* defined(MPL_VG_AVAILABLE) */


#endif /* !defined(MPL_VALGRIND_H_INCLUDED) */
