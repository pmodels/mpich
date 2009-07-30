/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_MEMDEFS_H
#define MPID_MEMDEFS_H
#include <mpichconf.h>
#include <mpimem.h>

#if defined(HAVE_GCC_AND_PENTIUM_ASM)
#define asm_memcpy(dst, src, n) do {                                                    \
        const char *_p = (char *)(src);                                                 \
        char *_q = (char *)(dst);                                                       \
        size_t _nl = (size_t)(n) >> 2;                                                  \
        __asm__ __volatile__ ("cld ; rep ; movsl ; movl %3,%0 ; rep ; movsb"            \
                              : "+c" (_nl), "+S" (_p), "+D" (_q)                        \
                              : "r" ((n) & 3) : "memory" );                             \
    } while (0)

/*
   nt_memcpy (dst, src, len)
   This performs a memcopy using non-temporal stores.  It's optimized
   for ia_32 machines.

   The general idea is to prefetch a block of the source data into the
   cache, then read the data from the source buffer into 64-bit mmx
   registers so that the data can be written to the destination buffer
   using non-temporal move instructions.

   This is done in three steps:  copy 8K or larger chunks, copy (8K,
   128B] chunks, and copy the rest.

   In the first step, the main loop prefetches an 8K chunk, by reading
   one element from each cacheline.  Then we copy that 8K chunk, 64
   bytes at a time (8bytes per mmx reg * 8 mmx regs) using
   non-temporal stores.  Rinse and repeat.

   The second step is essentially the same as the first, except that
   the amount of data to be copied in that step is less than 8K, so we
   prefetch all of the data.  These two steps could have been combined
   but I think I saved some time by simplifying the main loop in step
   one by not checking if we have to prefetch less than 8K.

   The last step just copies whatever's left.
   
 */

static inline void nt_memcpy (volatile void *dst, volatile const void *src, size_t len)
{
    void *dummy_dst;
    void *dummy_src;
    
    int n;

    /* copy in 8K chunks */
    n = len & (-8*1024);
    if (n)
    {

	__asm__ __volatile__ ("mov %4, %%ecx\n"
		      ".set PREFETCHBLOCK, 1024\n" /* prefetch PREFETCHBLOCK number of 8-byte words */
		      "lea (%%esi, %%ecx, 8), %%esi\n"
		      "lea (%%edi, %%ecx, 8), %%edi\n"
		  
		      "neg %%ecx\n"
		      "emms\n"
		  
		      "1:\n" /* main loop */

		      /* eax is the prefetch loop iteration counter */
		      "mov $PREFETCHBLOCK/16, %%eax\n"  /* only need to touch one element per cacheline, and we're doing two at once  */

		      /* prefetch 2 cachelines at a time (128 bytes) */
		      "2:\n" /* prefetch loop */
		      "mov (%%esi, %%ecx, 8), %%edx\n"
		      "mov 64(%%esi, %%ecx, 8), %%edx\n"
		      "add $16, %%ecx\n" 
		  
		      "dec %%eax\n"
		      "jnz 2b\n"
		      "sub $PREFETCHBLOCK, %%ecx\n"

		      /* eax is the copy loop iteration counter */
		      "mov $PREFETCHBLOCK/8, %%eax\n"

		      /* copy data 64 bytes at a time */
		      "3:\n" /* copy loop */
		      "movq (%%esi, %%ecx, 8), %%mm0\n"
		      "movq 8(%%esi, %%ecx, 8), %%mm1\n"
		      "movq 16(%%esi, %%ecx, 8), %%mm2\n"
		      "movq 24(%%esi, %%ecx, 8), %%mm3\n"
		      "movq 32(%%esi, %%ecx, 8), %%mm4\n"
		      "movq 40(%%esi, %%ecx, 8), %%mm5\n"
		      "movq 48(%%esi, %%ecx, 8), %%mm6\n"
		      "movq 56(%%esi, %%ecx, 8), %%mm7\n"
		  
		      "movntq %%mm0, (%%edi, %%ecx, 8)\n"
		      "movntq %%mm1, 8(%%edi, %%ecx, 8)\n"
		      "movntq %%mm2, 16(%%edi, %%ecx, 8)\n"
		      "movntq %%mm3, 24(%%edi, %%ecx, 8)\n"
		      "movntq %%mm4, 32(%%edi, %%ecx, 8)\n"
		      "movntq %%mm5, 40(%%edi, %%ecx, 8)\n"
		      "movntq %%mm6, 48(%%edi, %%ecx, 8)\n"
		      "movntq %%mm7, 56(%%edi, %%ecx, 8)\n"

		      "add $8, %%ecx\n"
		      "dec %%eax\n"
		      "jnz 3b\n"

		      "or %%ecx, %%ecx\n"
		      "jnz 1b\n"

		      "sfence\n"
		      "emms\n"
		      : "=D" (dummy_dst), "=S" (dummy_src)
		      : "0" (dst), "1" (src), "g" (n >> 3)
		      : "eax", "edx", "ecx", "memory" );

	src = (char *)src + n;
	dst = (char *)dst + n;
    }
    
    /* copy in 128byte chunks */
    n = len & (8*1024 - 1) & -128;
    if (n)
    {

	__asm__ __volatile__ ("mov %4, %%ecx\n"
		      "lea (%%esi, %%ecx, 8), %%esi\n"
		      "lea (%%edi, %%ecx, 8), %%edi\n"

		      "push %%ecx\n"        /* save n */
			  
		      "mov %%ecx, %%eax\n" /* prefetch loopctr = n/128 */
		      "shr $4, %%eax\n" 
		  
		      "neg %%ecx\n"
		      "emms\n"

		      /* prefetch all data to be copied 2 cachelines at a time (128 bytes)*/
		      "1:\n" /* prefetch loop */
		      "mov (%%esi, %%ecx, 8), %%edx\n"
		      "mov 64(%%esi, %%ecx, 8), %%edx\n"
		      "add $16, %%ecx\n"
		  
		      "dec %%eax\n"
		      "jnz 1b\n"

		      "pop %%ecx\n" /* restore n */

		      "mov %%ecx, %%eax\n" /* write loopctr = n/64 */
		      "shr $3, %%eax\n"
		      "neg %%ecx\n"

		      /* copy data 64 bytes at a time */
		      "2:\n" /* copy loop */
		      "movq (%%esi, %%ecx, 8), %%mm0\n"
		      "movq 8(%%esi, %%ecx, 8), %%mm1\n"
		      "movq 16(%%esi, %%ecx, 8), %%mm2\n"
		      "movq 24(%%esi, %%ecx, 8), %%mm3\n"
		      "movq 32(%%esi, %%ecx, 8), %%mm4\n"
		      "movq 40(%%esi, %%ecx, 8), %%mm5\n"
		      "movq 48(%%esi, %%ecx, 8), %%mm6\n"
		      "movq 56(%%esi, %%ecx, 8), %%mm7\n"
		  
		      "movntq %%mm0, (%%edi, %%ecx, 8)\n"
		      "movntq %%mm1, 8(%%edi, %%ecx, 8)\n"
		      "movntq %%mm2, 16(%%edi, %%ecx, 8)\n"
		      "movntq %%mm3, 24(%%edi, %%ecx, 8)\n"
		      "movntq %%mm4, 32(%%edi, %%ecx, 8)\n"
		      "movntq %%mm5, 40(%%edi, %%ecx, 8)\n"
		      "movntq %%mm6, 48(%%edi, %%ecx, 8)\n"
		      "movntq %%mm7, 56(%%edi, %%ecx, 8)\n"

		      "add $8, %%ecx\n"
		      "dec %%eax\n"
		      "jnz 2b\n"

		      "sfence\n"
		      "emms\n"
		      : "=D" (dummy_dst), "=S" (dummy_src) 
		      : "0" (dst), "1" (src), "g" (n >> 3)
		      : "eax", "edx", "ecx", "memory" );
	src = (char *)src + n;
	dst = (char *)dst + n;
    }
    
    /* copy leftover */
    n = len & (128 - 1);
    if (n)
	asm_memcpy (dst, src, n);    
}

#define MPID_NEM_MEMCPY_CROSSOVER (63*1024)

#define MPIU_Memcpy(a,b,c)  do {                                                                \
        MPIU_MEM_CHECK_MEMCPY((a),(b),(c));                                                     \
        if (((c)) >= MPID_NEM_MEMCPY_CROSSOVER)                                                 \
            nt_memcpy (a, b, c);                                                                \
        else                                                                                    \
            asm_memcpy (a, b, c);                                                               \
    } while (0)

#elif 0 && defined(HAVE_GCC_AND_X86_64_ASM)

#define asm_memcpy(dst, src, n)  do {                                                            \
        const char *_p = (char *)(src);                                                          \
        char *_q = (char *)(dst);                                                                \
        size_t _nq = n >> 3;                                                                     \
        __asm__ __volatile__ ("cld ; rep ; movsq ; movl %3,%%ecx ; rep ; movsb"                  \
                              : "+c" (_nq), "+S" (_p), "+D" (_q)                                 \
                              : "r" ((uint32_t)((n) & 7)) : "memory" );                          \
    } while (0)

static inline void amd64_cpy_nt (volatile void *dst, const volatile void *src, size_t n)
{
    size_t n32 = (n) >> 5;
    size_t nleft = (n) & (32-1);
    
    if (n32)
    {
	__asm__ __volatile__ (".align 16  \n"
		      "1:  \n"
		      "mov (%1), %%r8  \n"
		      "mov 8(%1), %%r9  \n"
		      "add $32, %1  \n"
		      "movnti %%r8, (%2)  \n"
		      "movnti %%r9, 8(%2)  \n"
		      "add $32, %2  \n"
		      "mov -16(%1), %%r8  \n"
		      "mov -8(%1), %%r9  \n"
		      "dec %0  \n"
		      "movnti %%r8, -16(%2)  \n"
		      "movnti %%r9, -8(%2)  \n"
		      "jnz 1b  \n"
		      "sfence  \n"
		      "mfence  \n"
		      : "+a" (n32), "+S" (src), "+D" (dst)
		      : : "r8", "r9", "memory" );
    }
    
    if (nleft)
    {
	memcpy ((void *)dst, (void *)src, nleft);
    }
}

static inline
void volatile_memcpy (volatile void *restrict dst, volatile const void *restrict src, size_t n)
{
    MPIUI_Memcpy ((void *)dst, (const void *)src, n);
}

#define MPID_NEM_MEMCPY_CROSSOVER (32*1024)
#define MPIU_Memcpy(a,b,c) do {                 \
        MPIU_MEM_CHECK_MEMCPY((a),(b),(c));     \
        if ((c) >= MPID_NEM_MEMCPY_CROSSOVER)   \
            amd64_cpy_nt(a, b, c);              \
        else                                    \
            volatile_memcpy(a, b, c);           \
    } while (0)
/* #define MPID_NEM_MEMCPY(a,b,c) (((c) < MPID_NEM_MEMCPY_CROSSOVER) ? memcpy(a, b, c) : amd64_cpy_nt(a, b, c)) */
/* #define MPID_NEM_MEMCPY(a,b,c) amd64_cpy_nt(a, b, c) */
/* #define MPID_NEM_MEMCPY(a,b,c) memcpy (a, b, c) */

#else
/* #define MPIU_Memcpy(dst, src, n) do { volatile void * restrict d = (dst); volatile const void *restrict s = (src); MPIUI_Memcpy((void *)d, (const void *)s, n); }while (0) */
#define MPIU_Memcpy(dst, src, n)                \
    do {                                        \
        MPIU_MEM_CHECK_MEMCPY((dst),(src),(n)); \
        MPIUI_Memcpy(dst, src, n);              \
    } while (0)
#endif

#endif /* MPID_MEMDEFS_H */
