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
/* FIXME this should probably be a configure/winconfigure test instead. */
#  define MPIDU_Shm_write_barrier()      MPID_Abort(MPIR_Process.comm_self, MPI_ERR_OTHER, 1, "memory barriers not implemented on this platform")
#  define MPIDU_Shm_read_barrier()       MPID_Abort(MPIR_Process.comm_self, MPI_ERR_OTHER, 1, "memory barriers not implemented on this platform")
#  define MPIDU_Shm_read_write_barrier() MPID_Abort(MPIR_Process.comm_self, MPI_ERR_OTHER, 1, "memory barriers not implemented on this platform")
#endif

#endif /* defined(MPIDU_MEM_BARRIERS_H_INCLUDED) */
