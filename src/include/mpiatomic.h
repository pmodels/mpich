/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIIMPLATOMIC_H_INCLUDED
#define MPIIMPLATOMIC_H_INCLUDED

/* These macros currently never get called because USE_THREAD_IMPL is
 * never set to MPICH_THREAD_IMPL_NOT_IMPLEMENTED, and therefore the
 * MPIU_Object_add_ref etc macros never get set to use
 * MPID_Atomic_incr etc in mpiimpl.h.
 */
#if defined(MPICH_IS_THREADED) && defined(USE_ATOMIC_UPDATES)

/* These can be implemented using special assembly language operations
   on most processors.  If no such operation is available, then each
   object, in addition to the ref_count field, must have a thread-lock. 
   
   We also need to know if the decremented value is zero so that we can see if
   we must deallocate the object.  This fetch and decrement must be 
   atomic so that multiple threads don't decide that they were 
   responsible for setting the value to zero.

   FIXME: Who sets "USE_ATOMIC_UPDATES"?  How should we select alternate
   versions (e.g., other IA32 compilers, IA64, PowerPC, MIPS, etc.)
 */
#ifdef HAVE_GCC_AND_PENTIUM_ASM
#define MPID_Atomic_incr( count_ptr )					\
   __asm__ __volatile__ ( "lock; incl %0"				\
                         : "=m" (*count_ptr) :: "memory", "cc" )

#define MPID_Atomic_decr_flag( count_ptr, nzflag )			\
   __asm__ __volatile__ ( "xor %%eax,%%eax; lock; decl %0 ; setnz %%al"	\
                        : "=m" (*count_ptr) , "=a" (nzflag) :: "memory", "cc" )

#define MPID_Atomic_fetch_and_incr(count_ptr_, count_old_) do {         \
        (count_old_) = 1;                                               \
        __asm__ __volatile__ ("lock ; xaddl %0,%1"                      \
                              : "=r" (count_old_), "=m" (*count_ptr_)   \
                              :  "0" (count_old_),  "m" (*count_ptr_)); \
    } while (0)

/* The Intel Pentium Pro has a bug that can result in out-of-order stores.  
   The rest of the Intel x86 processors perform writes
   in order, with the exception of the non-temporal SSE instructions which we 
   don't use.  The IDT WinChip can be configured to
   perform out-of-order writes.  FIXME: Should this be a configure time or 
   runtime decision?  Right now it is neither and we
   assume stores are ordered for x86 processors. */
#if !defined(HAVE_X86_OOOSTORE)
#define MPID_Atomic_write_barrier()		\
{						\
    __asm__ __volatile__ ("": : :"memory");	\
}
#else
#define MPID_Atomic_write_barrier()					\
{									\
    __asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory");	\
}
#endif /* !defined(HAVE_X86_OOOSTORE) */
#else
#error "Atomic updates specified but no code for this platform"
#endif

#endif /* defined(USE_ATOMIC_UPDATES) */
#endif /* defined(MPIIMPLATOMIC_H) */
