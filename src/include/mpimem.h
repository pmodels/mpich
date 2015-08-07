/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIMEM_H_INCLUDED
#define MPIMEM_H_INCLUDED

#ifndef MPICHCONF_H_INCLUDED
#error 'mpimem.h requires that mpichconf.h be included first'
#endif

/* Make sure that we have the definitions for the malloc routines and size_t */
#include <stdio.h>
#include <stdlib.h>
/* strdup is often declared in string.h, so if we plan to redefine strdup, 
   we need to include string first.  That is done below, only in the
   case where we redefine strdup */

#if defined(__cplusplus)
extern "C" {
#endif

#include "mpichconf.h"
#include "mpl.h"

/* Define attribute as empty if it has no definition */
#ifndef ATTRIBUTE
#define ATTRIBUTE(a)
#endif

/* ------------------------------------------------------------------------- */
/* mpimem.h */
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
/* style: allow:fprintf:5 sig:0 */   /* For handle debugging ONLY */
/* style: allow:snprintf:1 sig:0 */

/*D
  Memory - Memory Management Routines

  Rules for memory management:

  MPICH explicity prohibits the appearence of 'malloc', 'free', 
  'calloc', 'realloc', or 'strdup' in any code implementing a device or 
  MPI call (of course, users may use any of these calls in their code).  
  Instead, you must use 'MPIU_Malloc' etc.; if these are defined
  as 'malloc', that is allowed, but an explicit use of 'malloc' instead of
  'MPIU_Malloc' in the source code is not allowed.  This restriction is
  made to simplify the use of portable tools to test for memory leaks, 
  overwrites, and other consistency checks.

  Most memory should be allocated at the time that 'MPID_Init' is 
  called and released with 'MPID_Finalize' is called.  If at all possible,
  no other MPID routine should fail because memory could not be allocated
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
/* Safer string routines */
int MPIU_Strncpy( char *outstr, const char *instr, size_t maxlen );

int MPIU_Strnapp( char *, const char *, size_t );
char *MPIU_Strdup( const char * );

/* ---------------------------------------------------------------------- */
/* FIXME - The string routines do not belong in the memory header file  */
/* FIXME - The string error code such be MPICH-usable error codes */
#define MPIU_STR_SUCCESS    0
#define MPIU_STR_FAIL      -1
#define MPIU_STR_NOMEM      1

/* FIXME: Global types like this need to be discussed and agreed to */
typedef int MPIU_BOOL;

/* FIXME: These should be scoped to only the routines that need them */
#ifdef USE_HUMAN_READABLE_TOKENS

#define MPIU_STR_QUOTE_CHAR     '\"'
#define MPIU_STR_QUOTE_STR      "\""
#define MPIU_STR_DELIM_CHAR     '='
#define MPIU_STR_DELIM_STR      "="
#define MPIU_STR_ESCAPE_CHAR    '\\'
#define MPIU_STR_HIDE_CHAR      '*'
#define MPIU_STR_SEPAR_CHAR     ' '
#define MPIU_STR_SEPAR_STR      " "

#else

#define MPIU_STR_QUOTE_CHAR     '\"'
#define MPIU_STR_QUOTE_STR      "\""
#define MPIU_STR_DELIM_CHAR     '#'
#define MPIU_STR_DELIM_STR      "#"
#define MPIU_STR_ESCAPE_CHAR    '\\'
#define MPIU_STR_HIDE_CHAR      '*'
#define MPIU_STR_SEPAR_CHAR     '$'
#define MPIU_STR_SEPAR_STR      "$"

#endif

int MPIU_Str_get_string_arg(const char *str, const char *key, char *val, 
			    int maxlen);
int MPIU_Str_get_binary_arg(const char *str, const char *key, char *buffer, 
			    int maxlen, int *out_length);
int MPIU_Str_get_int_arg(const char *str, const char *key, int *val_ptr);
int MPIU_Str_add_string_arg(char **str_ptr, int *maxlen_ptr, const char *key, 
			    const char *val);
int MPIU_Str_add_binary_arg(char **str_ptr, int *maxlen_ptr, const char *key, 
			    const char *buffer, int length);
int MPIU_Str_add_int_arg(char **str_ptr, int *maxlen_ptr, const char *key, 
			 int val);
MPIU_BOOL MPIU_Str_hide_string_arg(char *str, const char *key);
int MPIU_Str_add_string(char **str_ptr, int *maxlen_ptr, const char *val);
int MPIU_Str_get_string(char **str_ptr, char *val, int maxlen);

/* ------------------------------------------------------------------------- */

void MPIU_trinit(int);
void *MPIU_trmalloc(size_t, int, const char []);
void MPIU_trfree(void *, int, const char []);
int MPIU_trvalid(const char []);
void MPIU_trspace(size_t *, size_t *);
void MPIU_trid(int);
void MPIU_trlevel(int);
void MPIU_trDebugLevel(int);
void *MPIU_trcalloc(size_t, size_t, int, const char []);
void *MPIU_trrealloc(void *, size_t, int, const char[]);
void *MPIU_trstrdup(const char *, int, const char[]);
void MPIU_TrSetMaxMem(size_t);
void MPIU_trdump(FILE *, int);

#ifdef USE_MEMORY_TRACING
/*M
  MPIU_Malloc - Allocate memory

  Synopsis:
.vb
  void *MPIU_Malloc( size_t len )
.ve

  Input Parameter:
. len - Length of memory to allocate in bytes

  Return Value:
  Pointer to allocated memory, or null if memory could not be allocated.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPIU_Malloc(n) malloc(n)
.ve
  However, it can also be defined as 
.vb
  #define MPIU_Malloc(n) MPIU_trmalloc(n,__LINE__,__FILE__)
.ve
  where 'MPIU_trmalloc' is a tracing version of 'malloc' that is included with 
  MPICH.

  Module:
  Utility
  M*/
#define MPIU_Malloc(a)    MPIU_trmalloc((a),__LINE__,__FILE__)

/*M
  MPIU_Calloc - Allocate memory that is initialized to zero.

  Synopsis:
.vb
    void *MPIU_Calloc( size_t nelm, size_t elsize )
.ve

  Input Parameters:
+ nelm - Number of elements to allocate
- elsize - Size of each element.

  Notes:
  Like 'MPIU_Malloc' and 'MPIU_Free', this will often be implemented as a 
  macro but may use 'MPIU_trcalloc' to provide a tracing version.

  Module:
  Utility
  M*/
#define MPIU_Calloc(a,b)  \
    MPIU_trcalloc((a),(b),__LINE__,__FILE__)

/*M
  MPIU_Free - Free memory

  Synopsis:
.vb
   void MPIU_Free( void *ptr )
.ve

  Input Parameter:
. ptr - Pointer to memory to be freed.  This memory must have been allocated
  with 'MPIU_Malloc'.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPIU_Free(n) free(n)
.ve
  However, it can also be defined as 
.vb
  #define MPIU_Free(n) MPIU_trfree(n,__LINE__,__FILE__)
.ve
  where 'MPIU_trfree' is a tracing version of 'free' that is included with 
  MPICH.

  Module:
  Utility
  M*/
#define MPIU_Free(a)      MPIU_trfree(a,__LINE__,__FILE__)

#define MPIU_Strdup(a)    MPIU_trstrdup(a,__LINE__,__FILE__)

#define MPIU_Realloc(a,b)    MPIU_trrealloc((a),(b),__LINE__,__FILE__)

/* Define these as invalid C to catch their use in the code */
#define malloc(a)         'Error use MPIU_Malloc' :::
#define calloc(a,b)       'Error use MPIU_Calloc' :::
#define free(a)           'Error use MPIU_Free'   :::
#define realloc(a)        'Error use MPIU_Realloc' :::
#if defined(strdup) || defined(__strdup)
#undef strdup
#endif
    /* We include string.h first, so that if it contains a definition of 
     strdup, we won't have an obscure failure when a file include string.h
    later in the compilation process. */
#include <string.h>

    /* The ::: should cause the compiler to choke; the string 
       will give the explanation */
#undef strdup /* in case strdup is a macro */
#define strdup(a)         'Error use MPIU_Strdup' :::

#else /* USE_MEMORY_TRACING */
/* No memory tracing; just use native functions */
#define MPIU_Malloc(a)    malloc((size_t)(a))
#define MPIU_Calloc(a,b)  calloc((size_t)(a),(size_t)(b))
#define MPIU_Free(a)      free((void *)(a))
#define MPIU_Realloc(a,b)  realloc((void *)(a),(size_t)(b))

#ifdef HAVE_STRDUP
/* Watch for the case where strdup is defined as a macro by a header include */
# if defined(NEEDS_STRDUP_DECL) && !defined(strdup)
extern char *strdup( const char * );
# endif
#define MPIU_Strdup(a)    strdup(a)
#else
/* Don't define MPIU_Strdup, provide it in safestr.c */
#endif /* HAVE_STRDUP */

#endif /* USE_MEMORY_TRACING */


/* Memory allocation macros. See document. */

/* Standard macro for generating error codes.  We set the error to be
 * recoverable by default, but this can be changed. */
#ifdef HAVE_ERROR_CHECKING
#define MPIU_CHKMEM_SETERR(rc_,nbytes_,name_) \
     rc_=MPIR_Err_create_code( MPI_SUCCESS, \
          MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
          MPI_ERR_OTHER, "**nomem2", "**nomem2 %d %s", nbytes_, name_ )
#else
#define MPIU_CHKMEM_SETERR(rc_,nbytes_,name_) rc_=MPI_ERR_OTHER
#endif

    /* CHKPMEM_REGISTER is used for memory allocated within another routine */

/* Memory used and freed within the current scopy (alloca if feasible) */
/* Configure with --enable-alloca to set USE_ALLOCA */
#if defined(HAVE_ALLOCA) && defined(USE_ALLOCA)
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
/* Define decl with a dummy definition to allow us to put a semi-colon
   after the macro without causing the declaration block to end (restriction
   imposed by C) */
#define MPIU_CHKLMEM_DECL(n_) int dummy_ ATTRIBUTE((unused))
#define MPIU_CHKLMEM_FREEALL()
#define MPIU_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)alloca(nbytes_); \
    if (!(pointer_) && (nbytes_ > 0)) {	   \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#else
#define MPIU_CHKLMEM_DECL(n_) \
 void *(mpiu_chklmem_stk_[n_]); \
 int mpiu_chklmem_stk_sp_=0;\
 MPIU_AssertDeclValue(const int mpiu_chklmem_stk_sz_,n_)

#define MPIU_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPIU_Malloc(nbytes_); \
if (pointer_) { \
    MPIU_Assert(mpiu_chklmem_stk_sp_<mpiu_chklmem_stk_sz_);\
    mpiu_chklmem_stk_[mpiu_chklmem_stk_sp_++] = pointer_;\
 } else if (nbytes_ > 0) {				 \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#define MPIU_CHKLMEM_FREEALL() \
    do { while (mpiu_chklmem_stk_sp_ > 0) {\
       MPIU_Free( mpiu_chklmem_stk_[--mpiu_chklmem_stk_sp_] ); } } while(0)
#endif /* HAVE_ALLOCA */
#define MPIU_CHKLMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* In some cases, we need to allocate large amounts of memory. This can
   be a problem if alloca is used, as the available stack space may be small.
   This is the same approach for the temporary memory as is used when alloca
   is not available. */
#define MPIU_CHKLBIGMEM_DECL(n_) \
 void *(mpiu_chklbigmem_stk_[n_]);\
 int mpiu_chklbigmem_stk_sp_=0;\
 MPIU_AssertDeclValue(const int mpiu_chklbigmem_stk_sz_,n_)

#define MPIU_CHKLBIGMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPIU_Malloc(nbytes_); \
if (pointer_) { \
    MPIU_Assert(mpiu_chklbigmem_stk_sp_<mpiu_chklbigmem_stk_sz_);\
    mpiu_chklbigmem_stk_[mpiu_chklbigmem_stk_sp_++] = pointer_;\
 } else if (nbytes_ > 0) {				       \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#define MPIU_CHKLBIGMEM_FREEALL() \
    { while (mpiu_chklbigmem_stk_sp_ > 0) {\
       MPIU_Free( mpiu_chklbigmem_stk_[--mpiu_chklbigmem_stk_sp_] ); } }

#define MPIU_CHKLBIGMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLBIGMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKLBIGMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLBIGMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* Persistent memory that we may want to recover if something goes wrong */
#define MPIU_CHKPMEM_DECL(n_) \
 void *(mpiu_chkpmem_stk_[n_]) = { NULL };     \
 int mpiu_chkpmem_stk_sp_=0;\
 MPIU_AssertDeclValue(const int mpiu_chkpmem_stk_sz_,n_)
#define MPIU_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPIU_Malloc(nbytes_); \
if (pointer_) { \
    MPIU_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);\
    mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;\
 } else if (nbytes_ > 0) {				 \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#define MPIU_CHKPMEM_REGISTER(pointer_) \
    {MPIU_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);\
    mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;}
#define MPIU_CHKPMEM_REAP() \
    { while (mpiu_chkpmem_stk_sp_ > 0) {\
       MPIU_Free( mpiu_chkpmem_stk_[--mpiu_chkpmem_stk_sp_] ); } }
#define MPIU_CHKPMEM_COMMIT() \
    mpiu_chkpmem_stk_sp_ = 0
#define MPIU_CHKPMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* now the CALLOC version for zeroed memory */
#define MPIU_CHKPMEM_CALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_CALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKPMEM_CALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_CALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)
#define MPIU_CHKPMEM_CALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
    do {                                                                   \
        pointer_ = (type_)MPIU_Calloc(1, (nbytes_));                       \
        if (pointer_) {                                                    \
            MPIU_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);        \
            mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;          \
        }                                                                  \
        else if (nbytes_ > 0) {                                            \
            MPIU_CHKMEM_SETERR(rc_,nbytes_,name_);                         \
            stmt_;                                                         \
        }                                                                  \
    } while (0)

/* A special version for routines that only allocate one item */
#define MPIU_CHKPMEM_MALLOC1(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPIU_Malloc(nbytes_); \
    if (!(pointer_) && (nbytes_ > 0)) {	   \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}

/* Provides a easy way to use realloc safely and avoid the temptation to use
 * realloc unsafely (direct ptr assignment).  Zero-size reallocs returning NULL
 * are handled and are not considered an error. */
#define MPIU_REALLOC_OR_FREE_AND_JUMP(ptr_,size_,rc_) do { \
    void *realloc_tmp_ = MPIU_Realloc((ptr_), (size_)); \
    if ((size_) && !realloc_tmp_) { \
        MPIU_Free(ptr_); \
        MPIR_ERR_SETANDJUMP2(rc_,MPI_ERR_OTHER,"**nomem2","**nomem2 %d %s",(size_),MPL_QUOTE(ptr_)); \
    } \
    (ptr_) = realloc_tmp_; \
} while (0)
/* this version does not free ptr_ */
#define MPIU_REALLOC_ORJUMP(ptr_,size_,rc_) do { \
    void *realloc_tmp_ = MPIU_Realloc((ptr_), (size_)); \
    if (size_) \
        MPIR_ERR_CHKANDJUMP2(!realloc_tmp_,rc_,MPI_ERR_OTHER,"**nomem2","**nomem2 %d %s",(size_),MPL_QUOTE(ptr_)); \
    (ptr_) = realloc_tmp_; \
} while (0)

#if defined(HAVE_STRNCASECMP)
#   define MPIU_Strncasecmp strncasecmp
#elif defined(HAVE_STRNICMP)
#   define MPIU_Strncasecmp strnicmp
#else
/* FIXME: Provide a fallback function ? */
#   error "No function defined for case-insensitive strncmp"
#endif

/* MPIU_Basename(path, basename)
   This function finds the basename in a path (ala "man 1 basename").
   *basename will point to an element in path.
   More formally: This function sets basename to the character just after the last '/' in path.
*/
void MPIU_Basename(char *path, char **basename);

/* Evaluates to a boolean expression, true if the given byte ranges overlap,
 * false otherwise.  That is, true iff [a_,a_+a_len_) overlaps with [b_,b_+b_len_) */
#define MPIU_MEM_RANGES_OVERLAP(a_,a_len_,b_,b_len_) \
    ( ((char *)(a_) >= (char *)(b_) && ((char *)(a_) < ((char *)(b_) + (b_len_)))) ||  \
      ((char *)(b_) >= (char *)(a_) && ((char *)(b_) < ((char *)(a_) + (a_len_)))) )
#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))

/* May be used to perform sanity and range checking on memcpy and mempcy-like
   function calls.  This macro will bail out much like an MPIU_Assert if any of
   the checks fail. */
#define MPIU_MEM_CHECK_MEMCPY(dst_,src_,len_)                                                                   \
    do {                                                                                                        \
        if (len_) {                                                                                             \
            MPIU_Assert((dst_) != NULL);                                                                        \
            MPIU_Assert((src_) != NULL);                                                                        \
            MPL_VG_CHECK_MEM_IS_ADDRESSABLE((dst_),(len_));                                                     \
            MPL_VG_CHECK_MEM_IS_ADDRESSABLE((src_),(len_));                                                     \
            if (MPIU_MEM_RANGES_OVERLAP((dst_),(len_),(src_),(len_))) {                                          \
                MPIU_Assert_fmt_msg(FALSE,("memcpy argument memory ranges overlap, dst_=%p src_=%p len_=%ld\n", \
                                           (dst_), (src_), (long)(len_)));                                      \
            }                                                                                                   \
        }                                                                                                       \
    } while (0)
#else
#define MPIU_MEM_CHECK_MEMCPY(dst_,src_,len_) do {} while(0)
#endif

/* valgrind macros are now provided by MPL (via mpl.h included in mpiimpl.h) */

/* ------------------------------------------------------------------------- */
/* end of mpimem.h */
/* ------------------------------------------------------------------------- */

#if defined(__cplusplus)
}
#endif
#endif
