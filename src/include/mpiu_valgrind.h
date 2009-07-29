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
#  define MPIU_VG_CHECK_MEM_IS_ADDRESSABLE(addr_,len_) VALGRIND_CHECK_MEM_IS_ADDRESSABLE((addr_),(len_))
#  if defined(HAVE_MACRO_VA_ARGS)
#    define MPIU_VG_PRINTF_BACKTRACE(...) VALGRIND_PRINTF_BACKTRACE(__VA_ARGS__)
#  else
#    define MPIU_VG_PRINTF_BACKTRACE VALGRIND_PRINTF_BACKTRACE
#  endif
#else
#  define MPIU_VG_MAKE_MEM_DEFINED(addr_,len_)
#  define MPIU_VG_MAKE_MEM_NOACCESS(addr_,len_)
#  define MPIU_VG_MAKE_MEM_UNDEFINED(addr_,len_)
#  define MPIU_VG_CHECK_MEM_IS_ADDRESSABLE(addr_,len_)
#  if defined(HAVE_MACRO_VA_ARGS)
#    define MPIU_VG_PRINTF_BACKTRACE(...)
#  else
#    define MPIU_VG_PRINTF_BACKTRACE MPIU_VG_printf_do_nothing_func
static inline void MPIU_VG_printf_do_nothing_func(char *fmt, ...)
{
    /* do nothing */
}
#  endif
#endif /* defined(MPIU_VG_AVAILABLE) */


#endif /* !defined(MPICH_VALGRIND_H_INCLUDED) */

