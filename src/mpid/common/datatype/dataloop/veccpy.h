/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef MPIDI_ENABLE_INTERNAL_OPENMP
#include <omp.h>
#endif

#ifndef VECCPY_H
#define VECCPY_H

#ifdef HAVE_ANY_INT64_T_ALIGNEMENT
#define MPIR_ALIGN8_TEST(p1,p2)
#else
#define MPIR_ALIGN8_TEST(p1,p2) && (((DLOOP_VOID_PTR_CAST_TO_OFFSET p1 | DLOOP_VOID_PTR_CAST_TO_OFFSET p2) & 0x7) == 0)
#endif

#ifdef HAVE_ANY_INT32_T_ALIGNEMENT
#define MPIR_ALIGN4_TEST(p1,p2)
#else
#define MPIR_ALIGN4_TEST(p1,p2) && (((DLOOP_VOID_PTR_CAST_TO_OFFSET p1 | DLOOP_VOID_PTR_CAST_TO_OFFSET p2) & 0x3) == 0)
#endif

#define MPIDI_COPY_FROM_VEC(src,dest,stride,type,nelms,count) \
{ \
    if (!nelms) { \
        src = (char*) DLOOP_OFFSET_CAST_TO_VOID_PTR                        \
                      ((DLOOP_VOID_PTR_CAST_TO_OFFSET (src)) +         \
		       ((DLOOP_Offset) count * (DLOOP_Offset) stride)); \
    } \
    else if (stride % sizeof(type)) { \
        MPIDI_COPY_FROM_VEC_UNALIGNED(src,dest,stride,type,nelms,count); \
    } \
    else { \
        MPIDI_COPY_FROM_VEC_ALIGNED(src,dest,stride/(DLOOP_Offset)sizeof(type),type,nelms,count); \
    } \
}

#define MPIDI_COPY_TO_VEC(src,dest,stride,type,nelms,count) \
{ \
    if (!nelms) { \
        dest = (char*) DLOOP_OFFSET_CAST_TO_VOID_PTR                        \
                       ((DLOOP_VOID_PTR_CAST_TO_OFFSET (dest)) +        \
                        ((DLOOP_Offset) count * (DLOOP_Offset) stride)); \
    } \
    else if (stride % (DLOOP_Offset) sizeof(type)) { \
        MPIDI_COPY_TO_VEC_UNALIGNED(src,dest,stride,type,nelms,count); \
    } \
    else { \
        MPIDI_COPY_TO_VEC_ALIGNED(src,dest,stride/(DLOOP_Offset)sizeof(type),type,nelms,count); \
    } \
}

#ifdef MPIDI_ENABLE_INTERNAL_OPENMP
#define MPIDI_COPY_FROM_VEC_ALIGNED(src,dest,stride,type,nelms,count) MPIDI_COPY_FROM_VEC_ALIGNED_OMP(src,dest,stride,type,nelms,count)
#else
#define MPIDI_COPY_FROM_VEC_ALIGNED(src,dest,stride,type,nelms,count) MPIDI_COPY_FROM_VEC_ALIGNED_SERIAL(src,dest,stride,type,nelms,count)
#endif
                                                                   \
#define MPIDI_COPY_FROM_VEC_ALIGNED_OMP(src,dest,stride,type,nelms,count) \
{                                                                   \
    type * l_src = (type *) src, * l_dest = (type *) dest;          \
    type * tmp_src = l_src;                                         \
    int i, j_iter;                                                  \
    register int k;                                                 \
    register unsigned long _i, j, total_count = count*nelms;        \
    const DLOOP_Offset l_stride = stride;                           \
                                                                    \
    DLOOP_Assert(stride <= INT_MAX);                                \
    DLOOP_Assert(total_count <= INT_MAX);                           \
    DLOOP_Assert(nelms <= INT_MAX);                                 \
    char value[64];                                                 \
    int flag;                                                       \
                                                                    \
    if(MPIR_Process.comm_world->info!=NULL)                         \
        MPIR_Info_get_impl(MPIR_Process.comm_world->info, "Parallel_pack", 64, value, &flag);\
                                                                    \
    /*Check if the MPI_INFO key is set to "true" to enable parallel packing*/\
    if (flag && !strcmp(value, "true")) {                           \
        /*Check if packing is called from OpenMP parallel region or not*/\
        if(!omp_in_parallel()) {                                    \
            /*When call is not made from parallel region, it is safe to use omp parallel for*/\
            _Pragma("omp parallel for private(j_iter)")             \
            for(i=0; i<count; i++) {                                \
                type *tmp_src = l_src + i*l_stride;                 \
                type *tmp_dest = l_dest + i*nelms;                  \
                for (j_iter=0; j_iter<nelms; j_iter++)              \
                    tmp_dest[j_iter] = tmp_src[j_iter];             \
            }                                                       \
        }                                                           \
        else {                                                      \
            /*When call is made from parallel region, create OMP tasks*/\
            int num_tasks = omp_get_max_threads();                  \
            int task_id;                                            \
            int tsk_brk_point = count-(count/num_tasks)*num_tasks;  \
            int tsks_left = num_tasks-tsk_brk_point;                \
            int count1 = tsk_brk_point * ((count/num_tasks)+1);     \
            int count2 = count-count1;                              \
            _Pragma("omp single nowait")                            \
            {                                                       \
                for (task_id=0; task_id<tsk_brk_point; task_id++) { \
                    int begin = task_id*count1/tsk_brk_point;       \
                    int end = (task_id+1)*count1/tsk_brk_point;     \
                                                                    \
                    _Pragma("omp task firstprivate(begin,end)")     \
                    {                                               \
                        int i, j_iter;                              \
                        type * tmp_src;                             \
                        type * tmp_dest;                            \
                        for(i=begin; i<end; i++) {                  \
                            tmp_src = l_src+i*l_stride;             \
                            tmp_dest = l_dest + i*nelms;            \
                            for (j_iter=0; j_iter<nelms; j_iter++)  \
                                tmp_dest[j_iter] = tmp_src[j_iter]; \
                        }                                           \
                    }                                               \
                }                                                   \
                for (task_id=0; task_id<tsks_left; task_id++) {     \
                    int begin = task_id*count2/tsks_left;           \
                    int end = (task_id+1)*count2/tsks_left;         \
                                                                    \
                    _Pragma("omp task firstprivate(begin,end)")     \
                    {                                               \
                        int i, j_iter;                              \
                        type * tmp_src;                             \
                        type * tmp_dest;                            \
                        for (i=begin; i<end; i++) {                 \
                            tmp_src = l_src + (i+count1)*l_stride;  \
                            tmp_dest = l_dest + (i+count1)*nelms;   \
                            for (j_iter=0; j_iter<nelms; j_iter++)  \
                                tmp_dest[j_iter] = tmp_src[j_iter]; \
                        }                                           \
                    }                                               \
                }                                                   \
            }                                                       \
            _Pragma("omp taskwait")                                 \
        }                                                           \
    }                                                               \
    else{                                                           \
    /*Falls back to original code if MPI_INFO key is not set*/      \
    MPIDI_COPY_FROM_VEC_ALIGNED_SERIAL(src,dest,stride,type,nelms,count)\
    }                                                               \
}

#define MPIDI_COPY_FROM_VEC_ALIGNED_SERIAL(src,dest,stride,type,nelms,count) \
{								\
    type * l_src = (type *) src, * l_dest = (type *) dest;	\
    type * tmp_src = l_src;                                     \
    register int k;                                             \
    register unsigned long _i, j;                               \
    unsigned long total_count = count * nelms;                  \
    const DLOOP_Offset l_stride = stride;                       \
                                                                \
    DLOOP_Assert(stride <= INT_MAX);                            \
    DLOOP_Assert(total_count <= INT_MAX);                       \
    DLOOP_Assert(nelms <= INT_MAX);                             \
    if (nelms == 1) {                                           \
        for (_i = (int)total_count; _i; _i--) {                 \
            *l_dest++ = *l_src;				        \
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else if (nelms == 2) {                                      \
        for (_i = (int)total_count; _i; _i -= 2) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else if (nelms == 3) {                                      \
        for (_i = (int)total_count; _i; _i -= 3) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else if (nelms == 4) {                                      \
        for (_i = (int)total_count; _i; _i -= 4) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else if (nelms == 5) {                                      \
        for (_i = (int)total_count; _i; _i -= 5) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else if (nelms == 6) {                                      \
        for (_i = (int)total_count; _i; _i -= 6) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else if (nelms == 7) {                                      \
        for (_i = (int)total_count; _i; _i -= 7) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else if (nelms == 8) {                                      \
        for (_i = (int)total_count; _i; _i -= 8) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            *l_dest++ = l_src[7];				\
            l_src += l_stride;                                  \
        }							\
    }                                                           \
    else {                                                      \
        _i = (int)total_count;                                  \
        while (_i) {                                             \
            tmp_src = l_src;                                    \
            j = (int)nelms;                                     \
            while (j >= 8) {                                    \
                *l_dest++ = tmp_src[0];				\
                *l_dest++ = tmp_src[1];				\
                *l_dest++ = tmp_src[2];				\
                *l_dest++ = tmp_src[3];				\
                *l_dest++ = tmp_src[4];				\
                *l_dest++ = tmp_src[5];				\
                *l_dest++ = tmp_src[6];				\
                *l_dest++ = tmp_src[7];				\
                j -= 8;                                         \
                tmp_src += 8;                                   \
            }                                                   \
            for (k = 0; k < j; k++) {                           \
                *l_dest++ = *tmp_src++;                         \
            }                                                   \
            l_src += l_stride;                                  \
            _i -= nelms;                                         \
        }                                                       \
    }                                                           \
    src = (char *) l_src;                                       \
    dest = (char *) l_dest;                                     \
}

#define MPIDI_COPY_FROM_VEC_UNALIGNED(src,dest,stride,type,nelms,count) \
{								\
    type * l_src = (type *) src, * l_dest = (type *) dest;	\
    type * tmp_src = l_src;                                     \
    register int k;                                                     \
    register unsigned long _i, j, total_count = count * nelms;          \
    const DLOOP_Offset l_stride = stride;				\
                                                                \
    DLOOP_Assert(stride <= INT_MAX);                            \
    DLOOP_Assert(total_count <= INT_MAX);                       \
    DLOOP_Assert(nelms <= INT_MAX);                             \
    if (nelms == 1) {                                           \
        for (_i = (int)total_count; _i; _i--) {                 \
            *l_dest++ = *l_src;				        \
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 2) {                                      \
        for (_i = (int)total_count; _i; _i -= 2) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 3) {                                      \
        for (_i = (int)total_count; _i; _i -= 3) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 4) {                                      \
        for (_i = (int)total_count; _i; _i -= 4) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 5) {                                      \
        for (_i = (int)total_count; _i; _i -= 5) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 6) {                                      \
        for (_i = (int)total_count; _i; _i -= 6) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 7) {                                      \
        for (_i = (int)total_count; _i; _i -= 7) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 8) {                                      \
        for (_i = (int)total_count; _i; _i -= 8) {              \
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            *l_dest++ = l_src[7];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else {                                                      \
        _i = (int)total_count;                                  \
        while (_i) {                                             \
            tmp_src = l_src;                                    \
            j = (int)nelms;                                     \
            while (j >= 8) {                                    \
                *l_dest++ = tmp_src[0];				\
                *l_dest++ = tmp_src[1];				\
                *l_dest++ = tmp_src[2];				\
                *l_dest++ = tmp_src[3];				\
                *l_dest++ = tmp_src[4];				\
                *l_dest++ = tmp_src[5];				\
                *l_dest++ = tmp_src[6];				\
                *l_dest++ = tmp_src[7];				\
                j -= 8;                                         \
                tmp_src += 8;                                   \
            }                                                   \
            for (k = 0; k < j; k++) {                           \
                *l_dest++ = *tmp_src++;                         \
            }                                                   \
            l_src = (type *) ((char *) l_src + l_stride);	\
            _i -= nelms;                                         \
        }                                                       \
    }                                                           \
    src = (char *) l_src;                                       \
    dest = (char *) l_dest;                                     \
}

#define MPIDI_COPY_TO_VEC_ALIGNED(src,dest,stride,type,nelms,count) \
{								\
    type * l_src = (type *) src, * l_dest = (type *) dest;	\
    type * tmp_dest = l_dest;                                   \
    register int k;                                             \
    register unsigned long _i, j;                               \
    unsigned long total_count = count * nelms;                  \
    const DLOOP_Offset l_stride = stride;				\
                                                                \
    DLOOP_Assert(stride <= INT_MAX);                            \
    DLOOP_Assert(total_count <= INT_MAX);                       \
    DLOOP_Assert(nelms <= INT_MAX);                             \
    if (nelms == 1) {                                           \
        for (_i = (int)total_count; _i; _i--) {                 \
            *l_dest = *l_src++;				        \
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else if (nelms == 2) {                                      \
        for (_i = (int)total_count; _i; _i -= 2) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else if (nelms == 3) {                                      \
        for (_i = (int)total_count; _i; _i -= 3) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else if (nelms == 4) {                                      \
        for (_i = (int)total_count; _i; _i -= 4) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else if (nelms == 5) {                                      \
        for (_i = (int)total_count; _i; _i -= 5) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else if (nelms == 6) {                                      \
        for (_i = (int)total_count; _i; _i -= 6) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else if (nelms == 7) {                                      \
        for (_i = (int)total_count; _i; _i -= 7) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else if (nelms == 8) {                                      \
        for (_i = (int)total_count; _i; _i -= 8) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            l_dest[7] = *l_src++;				\
            l_dest += l_stride;                                 \
        }							\
    }                                                           \
    else {                                                      \
        _i = (int)total_count;                                  \
        while (_i) {                                             \
            tmp_dest = l_dest;                                  \
            j = (int)nelms;                                     \
            while (j >= 8) {                                    \
                tmp_dest[0] = *l_src++;				\
                tmp_dest[1] = *l_src++;				\
                tmp_dest[2] = *l_src++;				\
                tmp_dest[3] = *l_src++;				\
                tmp_dest[4] = *l_src++;				\
                tmp_dest[5] = *l_src++;				\
                tmp_dest[6] = *l_src++;				\
                tmp_dest[7] = *l_src++;				\
                j -= 8;                                         \
                tmp_dest += 8;                                  \
            }                                                   \
            for (k = 0; k < j; k++) {                           \
                *tmp_dest++ = *l_src++;                         \
            }                                                   \
            l_dest += l_stride;                                 \
            _i -= nelms;                                         \
        }                                                       \
    }                                                           \
    src = (char *) l_src;                                       \
    dest = (char *) l_dest;                                     \
}

#define MPIDI_COPY_TO_VEC_UNALIGNED(src,dest,stride,type,nelms,count) \
{								\
    type * l_src = (type *) src, * l_dest = (type *) dest;	\
    type * tmp_dest = l_dest;                                   \
    register int k;                                             \
    register unsigned long _i, j;                               \
    unsigned long total_count = count * nelms;                  \
    const DLOOP_Offset l_stride = stride;                       \
                                                                \
    DLOOP_Assert(stride <= INT_MAX);                            \
    DLOOP_Assert(total_count <= INT_MAX);                       \
    DLOOP_Assert(nelms <= INT_MAX);                             \
    if (nelms == 1) {                                           \
        for (_i = (int)total_count; _i; _i--) {                 \
            *l_dest = *l_src++;				        \
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 2) {                                      \
        for (_i = (int)total_count; _i; _i -= 2) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 3) {                                      \
        for (_i = (int)total_count; _i; _i -= 3) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 4) {                                      \
        for (_i = (int)total_count; _i; _i -= 4) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 5) {                                      \
        for (_i = (int)total_count; _i; _i -= 5) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 6) {                                      \
        for (_i = (int)total_count; _i; _i -= 6) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 7) {                                      \
        for (_i = (int)total_count; _i; _i -= 7) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 8) {                                      \
        for (_i = (int)total_count; _i; _i -= 8) {              \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            l_dest[7] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else {                                                      \
        _i = (int)total_count;                                  \
        while (_i) {                                             \
            tmp_dest = l_dest;                                  \
            j = (int)nelms;                                     \
            while (j >= 8) {                                    \
                tmp_dest[0] = *l_src++;				\
                tmp_dest[1] = *l_src++;				\
                tmp_dest[2] = *l_src++;				\
                tmp_dest[3] = *l_src++;				\
                tmp_dest[4] = *l_src++;				\
                tmp_dest[5] = *l_src++;				\
                tmp_dest[6] = *l_src++;				\
                tmp_dest[7] = *l_src++;				\
                j -= 8;                                         \
                tmp_dest += 8;                                  \
            }                                                   \
            for (k = 0; k < j; k++) {                           \
                *tmp_dest++ = *l_src++;                         \
            }                                                   \
            l_dest = (type *) ((char *) l_dest + l_stride);	\
            _i -= nelms;                                         \
        }                                                       \
    }                                                           \
    src = (char *) l_src;                                       \
    dest = (char *) l_dest;                                     \
}

#endif /* VECCPY_H */

/*
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
