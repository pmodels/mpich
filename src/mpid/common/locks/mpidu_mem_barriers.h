/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_MEM_BARRIERS_H_INCLUDED
#define MPIDU_MEM_BARRIERS_H_INCLUDED

/* TODO fill more of these in, including versions that use the AO lib */

#include <mpichconf.h>

#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
#  define MPIDU_Shm_write_barrier() __asm__ __volatile__  ( "sfence" ::: "memory" )
#  define MPIDU_Shm_read_barrier() __asm__ __volatile__  ( "lfence" ::: "memory" )
#  define MPIDU_Shm_read_write_barrier() __asm__ __volatile__  ( "mfence" ::: "memory" )
#elif defined(HAVE_MASM_AND_X86)
#  define MPIDU_Shm_write_barrier()
#  define MPIDU_Shm_read_barrier() __asm { __asm _emit 0x0f __asm _emit 0xae __asm _emit 0xe8 }
#  define MPIDU_Shm_read_write_barrier()
#elif defined(HAVE_GCC_AND_IA64_ASM)
#  define MPIDU_Shm_write_barrier() __asm__ __volatile__  ("mf" ::: "memory" )
#  define MPIDU_Shm_read_barrier() __asm__ __volatile__  ("mf" ::: "memory" )
#  define MPIDU_Shm_read_write_barrier() __asm__ __volatile__  ("mf" ::: "memory" )
#else
   /* FIXME need to remove #warning, since it's not standard C.  However, we
      can't just replace this with #error because all non-intel platforms will
      stop compiling.  Saying nothing is also bad, since we're likely to see
      corruption and other strange bugs. [goodell@ 2008-03-11] */
#  warning No memory barrier definition present, defaulting to nop barriers
#  define MPIDU_Shm_write_barrier()
#  define MPIDU_Shm_read_barrier()
#  define MPIDU_Shm_read_write_barrier()
#endif

#endif /* defined(MPIDU_MEM_BARRIERS_H_INCLUDED) */
