/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIR_MEM_H_INCLUDED
#define MPIR_MEM_H_INCLUDED

#include "mpichconf.h"

/* Make sure that we have the definitions for the malloc routines and size_t */
#include <stdio.h>
#include <stdlib.h>
/* strdup is often declared in string.h, so if we plan to redefine strdup,
   we need to include string first.  That is done below, only in the
   case where we redefine strdup */

#if defined(__cplusplus)
extern "C" {
#endif

#include "mpl.h"

/* Define attribute as empty if it has no definition */
#ifndef ATTRIBUTE
#define ATTRIBUTE(a)
#endif

#if defined (MPL_USE_DBG_LOGGING)
    extern MPL_dbg_class MPIR_DBG_STRING;
#endif                          /* MPL_USE_DBG_LOGGING */

/* ------------------------------------------------------------------------- */
/* mpir_mem.h */
/* ------------------------------------------------------------------------- */
/* Memory allocation */
/* style: allow:malloc:2 sig:0 */
/* style: allow:free:2 sig:0 */
/* style: allow:strdup:2 sig:0 */
/* style: allow:calloc:2 sig:0 */
/* style: allow:realloc:1 sig:0 */
/* style: allow:alloca:1 sig:0 */
/* style: define:__strdup:1 sig:0 */
/* style: define:strdup:1 sig:0 */
    /* style: allow:fprintf:5 sig:0 *//* For handle debugging ONLY */
/* style: allow:snprintf:1 sig:0 */

/*D
  Memory - Memory Management Routines

  Rules for memory management:

  MPICH explicity prohibits the appearence of 'malloc', 'free',
  'calloc', 'realloc', or 'strdup' in any code implementing a device or
  MPI call (of course, users may use any of these calls in their code).
  Instead, you must use 'MPL_malloc' etc.; if these are defined
  as 'malloc', that is allowed, but an explicit use of 'malloc' instead of
  'MPL_malloc' in the source code is not allowed.  This restriction is
  made to simplify the use of portable tools to test for memory leaks,
  overwrites, and other consistency checks.

  Most memory should be allocated at the time that 'MPID_Init' is
  called and released with 'MPID_Finalize' is called.  If at all possible,
  no other routine should fail because memory could not be allocated
  (for example, because the user has allocated large arrays after 'MPI_Init').

  The implementation of the MPI routines will strive to avoid memory allocation
  as well; however, operations such as 'MPI_Type_index' that create a new
  data type that reflects data that must be copied from an array of arbitrary
  size will have to allocate memory (and can fail; note that there is an
  MPI error class for out-of-memory).

  Question:
  Do we want to have an aligned allocation routine?  E.g., one that
  aligns memory on a cache-line.
  D*/

/* Define the string copy and duplication functions */
/* ------------------------------------------------------------------------- */

#define MPIR_Memcpy(dst, src, len)              \
    do {                                        \
        CHECK_MEMCPY((dst),(src),(len));        \
        memcpy((dst), (src), (len));            \
    } while (0)

#ifdef USE_MEMORY_TRACING

/* Define these as invalid C to catch their use in the code */
#define malloc(a)         'Error use MPL_malloc' :::
#define calloc(a,b)       'Error use MPL_calloc' :::
#define free(a)           'Error use MPL_free'   :::
#define realloc(a)        'Error use MPL_realloc' :::
/* These two functions can't be guarded because we use #include <sys/mman.h>
 * throughout the code to be able to use other symbols in that header file.
 * Because we include that header directly, we bypass this guard and cause
 * compile problems.
 * #define mmap(a,b,c,d,e,f) 'Error use MPL_mmap'   :::
 * #define munmap(a,b)       'Error use MPL_munmap' :::
 */
#if defined(strdup) || defined(__strdup)
#undef strdup
#endif                          /* defined(strdup) || defined(__strdup) */
    /* The ::: should cause the compiler to choke; the string
     * will give the explanation */
#undef strdup                   /* in case strdup is a macro */
#define strdup(a)         'Error use MPL_strdup' :::

#endif                          /* USE_MEMORY_TRACING */


/* Memory allocation macros. See document. */

/* Standard macro for generating error codes.  We set the error to be
 * recoverable by default, but this can be changed. */
#ifdef HAVE_ERROR_CHECKING
#define MPIR_CHKMEM_SETERR(rc_,nbytes_,name_)                           \
    rc_=MPIR_Err_create_code(MPI_SUCCESS,                               \
                             MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,    \
                             MPI_ERR_OTHER, "**nomem2", "**nomem2 %d %s", nbytes_, name_)
#else                           /* HAVE_ERROR_CHECKING */
#define MPIR_CHKMEM_SETERR(rc_,nbytes_,name_) rc_=MPI_ERR_OTHER
#endif                          /* HAVE_ERROR_CHECKING */

    /* CHKPMEM_REGISTER is used for memory allocated within another routine */

/* Memory used and freed within the current scope (alloca if feasible) */
/* Configure with --enable-alloca to set USE_ALLOCA */
#if defined(HAVE_ALLOCA) && defined(USE_ALLOCA)
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif                          /* HAVE_ALLOCA_H */
/* Define decl with a dummy definition to allow us to put a semi-colon
   after the macro without causing the declaration block to end (restriction
   imposed by C) */
#define MPIR_CHKLMEM_DECL(n_) int dummy_ ATTRIBUTE((unused))
#define MPIR_CHKLMEM_FREEALL()
#define MPIR_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,stmt_) \
    {                                                                   \
        pointer_ = (type_)alloca(nbytes_);                              \
        if (!(pointer_) && (nbytes_ > 0)) {                             \
            MPIR_CHKMEM_SETERR(rc_,nbytes_,name_);                      \
            stmt_;                                                      \
        }                                                               \
    }
#else                           /* defined(HAVE_ALLOCA) && defined(USE_ALLOCA) */
#define MPIR_CHKLMEM_DECL(n_)                                   \
    void *(mpiu_chklmem_stk_[n_]) = { NULL };                   \
    int mpiu_chklmem_stk_sp_=0;                                 \
    MPIR_AssertDeclValue(const int mpiu_chklmem_stk_sz_,n_)

#define MPIR_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,stmt_) \
    {                                                                   \
        pointer_ = (type_)MPL_malloc(nbytes_,class_);                   \
        if (pointer_) {                                                 \
            MPIR_Assert(mpiu_chklmem_stk_sp_<mpiu_chklmem_stk_sz_);     \
            mpiu_chklmem_stk_[mpiu_chklmem_stk_sp_++] = pointer_;       \
        } else if (nbytes_ > 0) {                                       \
            MPIR_CHKMEM_SETERR(rc_,nbytes_,name_);                      \
            stmt_;                                                      \
        }                                                               \
    }
#define MPIR_CHKLMEM_FREEALL()                                          \
    do {                                                                \
        while (mpiu_chklmem_stk_sp_ > 0) {                              \
            MPL_free(mpiu_chklmem_stk_[--mpiu_chklmem_stk_sp_]);        \
        }                                                               \
    } while (0)
#endif                          /* defined(HAVE_ALLOCA) && defined(USE_ALLOCA) */
#define MPIR_CHKLMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_,class_)    \
    MPIR_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_,class_)
#define MPIR_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_,class_) \
    MPIR_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,goto fn_fail)

/* Persistent memory that we may want to recover if something goes wrong */
#define MPIR_CHKPMEM_DECL(n_)                                   \
    void *(mpiu_chkpmem_stk_[n_]) = { NULL };                   \
    int mpiu_chkpmem_stk_sp_=0;                                 \
    MPIR_AssertDeclValue(const int mpiu_chkpmem_stk_sz_,n_)
#define MPIR_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,stmt_) \
    {                                                                   \
        pointer_ = (type_)MPL_malloc(nbytes_,class_);                   \
        if (pointer_) {                                                 \
            MPIR_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);     \
            mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;       \
        } else if (nbytes_ > 0) {                                       \
            MPIR_CHKMEM_SETERR(rc_,nbytes_,name_);                      \
            stmt_;                                                      \
        }                                                               \
    }
#define MPIR_CHKPMEM_REGISTER(pointer_)                         \
    {                                                           \
        MPIR_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_); \
        mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;   \
    }
#define MPIR_CHKPMEM_REAP()                                             \
    {                                                                   \
        while (mpiu_chkpmem_stk_sp_ > 0) {                              \
            MPL_free(mpiu_chkpmem_stk_[--mpiu_chkpmem_stk_sp_]);        \
        }                                                               \
    }
#define MPIR_CHKPMEM_COMMIT()                   \
    mpiu_chkpmem_stk_sp_ = 0
#define MPIR_CHKPMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_,class_)    \
    MPIR_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_,class_)
#define MPIR_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_,class_) \
    MPIR_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,goto fn_fail)

/* now the CALLOC version for zeroed memory */
#define MPIR_CHKPMEM_CALLOC(pointer_,type_,nbytes_,rc_,name_,class_)    \
    MPIR_CHKPMEM_CALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_,class_)
#define MPIR_CHKPMEM_CALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_,class_) \
    MPIR_CHKPMEM_CALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,goto fn_fail)
#define MPIR_CHKPMEM_CALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,stmt_) \
    do {                                                                \
        pointer_ = (type_)MPL_calloc(1, (nbytes_), (class_));           \
        if (pointer_) {                                                 \
            MPIR_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);     \
            mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;       \
        }                                                               \
        else if (nbytes_ > 0) {                                         \
            MPIR_CHKMEM_SETERR(rc_,nbytes_,name_);                      \
            stmt_;                                                      \
        }                                                               \
    } while (0)

/* A special version for routines that only allocate one item */
#define MPIR_CHKPMEM_MALLOC1(pointer_,type_,nbytes_,rc_,name_,class_,stmt_) \
    {                                                                   \
        pointer_ = (type_)MPL_malloc(nbytes_,class_);                   \
        if (!(pointer_) && (nbytes_ > 0)) {                             \
            MPIR_CHKMEM_SETERR(rc_,nbytes_,name_);                      \
            stmt_;                                                      \
        }                                                               \
    }

/* Provides a easy way to use realloc safely and avoid the temptation to use
 * realloc unsafely (direct ptr assignment).  Zero-size reallocs returning NULL
 * are handled and are not considered an error. */
#define MPIR_REALLOC_ORJUMP(ptr_,size_,class_,rc_)                      \
    do {                                                                \
        void *realloc_tmp_ = MPL_realloc((ptr_), (size_), (class_));    \
        if (size_ != 0)                                                 \
            MPIR_ERR_CHKANDJUMP2(!realloc_tmp_,rc_,MPI_ERR_OTHER,"**nomem2","**nomem2 %d %s",(size_),MPL_QUOTE(ptr_)); \
        (ptr_) = realloc_tmp_;                                          \
    } while (0)

#if defined(HAVE_STRNCASECMP)
#define MPIR_Strncasecmp strncasecmp
#elif defined(HAVE_STRNICMP)
#define MPIR_Strncasecmp strnicmp
#else
/* FIXME: Provide a fallback function ? */
#error "No function defined for case-insensitive strncmp"
#endif

/* Evaluates to a boolean expression, true if the given byte ranges overlap,
 * false otherwise.  That is, true iff [a_,a_+a_len_) overlaps with [b_,b_+b_len_) */
#define MPIR_MEM_RANGES_OVERLAP(a_,a_len_,b_,b_len_)                    \
    (((char *)(a_) >= (char *)(b_) && ((char *)(a_) < ((char *)(b_) + (b_len_)))) || \
     ((char *)(b_) >= (char *)(a_) && ((char *)(b_) < ((char *)(a_) + (a_len_)))))
#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))

/* May be used to perform sanity and range checking on memcpy and mempcy-like
   function calls.  This macro will bail out much like an MPIR_Assert if any of
   the checks fail. */
#define CHECK_MEMCPY(dst_,src_,len_)                                    \
    do {                                                                \
        if (len_ != 0) {                                                \
            MPL_VG_CHECK_MEM_IS_ADDRESSABLE((dst_),(len_));             \
            MPL_VG_CHECK_MEM_IS_ADDRESSABLE((src_),(len_));             \
            if (MPIR_MEM_RANGES_OVERLAP((dst_),(len_),(src_),(len_))) { \
                MPIR_Assert_fmt_msg(FALSE,("memcpy argument memory ranges overlap, dst_=%p src_=%p len_=%ld\n", \
                                           (dst_), (src_), (long)(len_))); \
            }                                                           \
        }                                                               \
    } while (0)
#else
#define CHECK_MEMCPY(dst_,src_,len_) do {} while (0)
#endif

/* valgrind macros are now provided by MPL (via mpl.h included in mpiimpl.h) */

/* ------------------------------------------------------------------------- */
/* end of mpir_mem.h */
/* ------------------------------------------------------------------------- */

#if defined(__cplusplus)
}
#endif
#endif                          /* MPIR_MEM_H_INCLUDED */
