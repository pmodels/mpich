/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SIMPLE_PMI_UTIL_H_INCLUDED
#define SIMPLE_PMI_UTIL_H_INCLUDED

#include "pmi2conf.h"

/* maximum sizes for arrays */
#define PMI2U_MAXLINE 1024
#define PMI2U_IDSIZE    32

/* prototypes for PMIU routines */
void PMI2U_Set_rank( int PMI_rank );
void PMI2U_SetServer( void );
void PMI2U_printf( int print_flag, const char *fmt, ... );
int  PMI2U_readline( int fd, char *buf, int max );
int  PMI2U_writeline( int fd, char *buf );
int  PMI2U_parse_keyvals( char *st );
void PMI2U_dump_keyvals( void );
char *PMI2U_getval( const char *keystr, char *valstr, int vallen );
void PMI2U_chgval( const char *keystr, char *valstr );

#ifdef HAVE__FUNCTION__
#define PMI2U_FUNC __FUNCTION__
#elif defined(HAVE_CAP__FUNC__)
#define PMI2U_FUNC __FUNC__
#elif defined(HAVE__FUNC__)
#define PMI2U_FUNC __func__
#else
#define PMI2U_FUNC __FILE__
#endif

extern int PMI2_pmiverbose; /* Set this to true to print PMI debugging info */
#define printf_d(x...)  do { if (PMI2_pmiverbose) printf(x); } while (0)

/* error reporting macros */

#define PMI2U_ERR_POP(err) do { pmi2_errno = err; printf_d("ERROR: %s (%d)\n", PMI2U_FUNC, __LINE__); goto fn_fail; } while (0)
#define PMI2U_ERR_SETANDJUMP(err, class, str) do {                              \
        printf_d("ERROR: "str" in %s (%d)\n", PMI2U_FUNC, __LINE__);     \
        pmi2_errno = class;                                                      \
        goto fn_fail;                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP1(err, class, str, str1, arg) do {                          \
        printf_d("ERROR: "str1" in %s (%d)\n", arg, PMI2U_FUNC, __LINE__);       \
        pmi2_errno = class;                                                              \
        goto fn_fail;                                                                   \
    } while (0)
#define PMI2U_ERR_SETANDJUMP2(err, class, str, str1, arg1, arg2) do {                           \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, PMI2U_FUNC, __LINE__);        \
        pmi2_errno = class;                                                                      \
        goto fn_fail;                                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP3(err, class, str, str1, arg1, arg2, arg3) do {                     \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, PMI2U_FUNC, __LINE__);  \
        pmi2_errno = class;                                                                      \
        goto fn_fail;                                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP4(err, class, str, str1, arg1, arg2, arg3, arg4) do {                       \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, arg4, PMI2U_FUNC, __LINE__);    \
        pmi2_errno = class;                                                                              \
        goto fn_fail;                                                                                   \
    } while (0)
#define PMI2U_ERR_SETANDJUMP5(err, class, str, str1, arg1, arg2, arg3, arg4, arg5) do {                         \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, arg4, arg5, PMI2U_FUNC, __LINE__);      \
        pmi2_errno = class;                                                                                      \
        goto fn_fail;                                                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP6(err, class, str, str1, arg1, arg2, arg3, arg4, arg5, arg6) do {                           \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, arg4, arg5, arg6, PMI2U_FUNC, __LINE__);        \
        pmi2_errno = class;                                                                                              \
        goto fn_fail;                                                                                                   \
    } while (0)


#define PMI2U_ERR_CHKANDJUMP(cond, err, class, str) do {        \
        if (cond)                                               \
            PMI2U_ERR_SETANDJUMP(err, class, str);              \
    } while (0)
#define PMI2U_ERR_CHKANDJUMP1(cond, err, class, str, str1, arg) do {    \
        if (cond)                                                       \
            PMI2U_ERR_SETANDJUMP1(err, class, str, str1, arg);          \
    } while (0)
#define PMI2U_ERR_CHKANDJUMP2(cond, err, class, str, str1, arg1, arg2) do {     \
        if (cond)                                                               \
            PMI2U_ERR_SETANDJUMP2(err, class, str, str1, arg1, arg2);           \
    } while (0)
#define PMI2U_ERR_CHKANDJUMP3(cond, err, class, str, str1, arg1, arg2, arg3) do {       \
        if (cond)                                                                       \
            PMI2U_ERR_SETANDJUMP3(err, class, str, str1, arg1, arg2, arg3);             \
    } while (0)
#define PMI2U_ERR_CHKANDJUMP4(cond, err, class, str, str1, arg1, arg2, arg3, arg4) do { \
        if (cond)                                                                       \
            PMI2U_ERR_SETANDJUMP4(err, class, str, str1, arg1, arg2, arg3, arg4);       \
    } while (0)
#define PMI2U_ERR_CHKANDJUMP5(cond, err, class, str, str1, arg1, arg2, arg3, arg4, arg5) do {   \
        if (cond)                                                                               \
            PMI2U_ERR_SETANDJUMP5(err, class, str, str1, arg1, arg2, arg3, arg4, arg5);         \
    } while (0)
#define PMI2U_ERR_CHKANDJUMP6(cond, err, class, str, str1, arg1, arg2, arg3, arg4, arg5, arg6) do {     \
        if (cond)                                                                                       \
            PMI2U_ERR_SETANDJUMP6(err, class, str, str1, arg1, arg2, arg3, arg4, arg5, arg6);           \
    } while (0)

#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))
#define PMI2U_AssertDecl(a_) a_
#define PMI2U_AssertDeclValue(_a, _b) _a = _b
#else
/* Empty decls not allowed in C */
#define PMI2U_AssertDecl(a_) a_ 
#define PMI2U_AssertDeclValue(_a, _b) _a ATTRIBUTE((unused))
#endif

#ifdef HAVE_ERROR_CHECKING
#define PMI2U_CHKMEM_SETERR(rc_, nbytes_, name_) do {                                           \
        rc_ = PMI2_ERR_NOMEM;                                                                   \
        printf_d("ERROR: memory allocation of %lu bytes failed for %s in %s (%d)\n",     \
                (size_t)nbytes_, name_, PMI2U_FUNC, __LINE__);                                  \
    } while(0)
#else
#define PMI2U_CHKMEM_SETERR(rc_, nbytes_, name_) rc_ = PMI2_ERR_NOMEM
#endif


#if defined(HAVE_ALLOCA) && defined(USE_ALLOCA)
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
/* Define decl with a dummy definition to allow us to put a semi-colon
   after the macro without causing the declaration block to end (restriction
   imposed by C) */
#define PMI2U_CHKLMEM_DECL(n_) int dummy_ ATTRIBUTE((unused))
#define PMI2U_CHKLMEM_FREEALL()
#define PMI2U_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) do {        \
        pointer_ = (type_)alloca(nbytes_);                                              \
        if (!(pointer_)) {                                                              \
            PMI2U_CHKMEM_SETERR(rc_,nbytes_,name_);                                     \
            stmt_;                                                                      \
        }                                                                               \
    } while(0)
#else
#define PMI2U_CHKLMEM_DECL(n_)                                  \
    void *(pmi2u_chklmem_stk_[n_]) = {0};                       \
    int pmi2u_chklmem_stk_sp_=0;                                \
    PMI2U_AssertDeclValue(const int pmi2u_chklmem_stk_sz_,n_)

#define PMI2U_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) do {        \
        pointer_ = (type_)PMI2U_Malloc(nbytes_);                                        \
        if (pointer_) {                                                                 \
            PMI2U_Assert(pmi2u_chklmem_stk_sp_<pmi2u_chklmem_stk_sz_);                  \
            pmi2u_chklmem_stk_[pmi2u_chklmem_stk_sp_++] = pointer_;                     \
        } else {                                                                        \
            PMI2U_CHKMEM_SETERR(rc_,nbytes_,name_);                                     \
            stmt_;                                                                      \
        }                                                                               \
    } while(0)
#define PMI2U_CHKLMEM_FREEALL()                                         \
    while (pmi2u_chklmem_stk_sp_ > 0) {                                 \
        PMI2U_Free( pmi2u_chklmem_stk_[--pmi2u_chklmem_stk_sp_] ); }
#endif /* HAVE_ALLOCA */
#define PMI2U_CHKLMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    PMI2U_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define PMI2U_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    PMI2U_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* In some cases, we need to allocate large amounts of memory. This can
   be a problem if alloca is used, as the available stack space may be small.
   This is the same approach for the temporary memory as is used when alloca
   is not available. */
#define PMI2U_CHKLBIGMEM_DECL(n_)                                       \
    void *(pmi2u_chklbigmem_stk_[n_]);                                  \
    int pmi2u_chklbigmem_stk_sp_ = 0;                                   \
    PMI2U_AssertDeclValue(const int pmi2u_chklbigmem_stk_sz_,n_)

#define PMI2U_CHKLBIGMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) do {     \
        pointer_ = (type_)PMI2U_Malloc(nbytes_);                                        \
        if (pointer_) {                                                                 \
            PMI2U_Assert(pmi2u_chklbigmem_stk_sp_<pmi2u_chklbigmem_stk_sz_);            \
            pmi2u_chklbigmem_stk_[pmi2u_chklbigmem_stk_sp_++] = pointer_;               \
        } else {                                                                        \
            PMI2U_CHKMEM_SETERR(rc_,nbytes_,name_);                                     \
            stmt_;                                                                      \
        }                                                                               \
    } while(0)
#define PMI2U_CHKLBIGMEM_FREEALL()                                              \
    while (pmi2u_chklbigmem_stk_sp_ > 0) {                                      \
        PMI2U_Free( pmi2u_chklbigmem_stk_[--pmi2u_chklbigmem_stk_sp_] ); }

#define PMI2U_CHKLBIGMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_)       \
    PMI2U_CHKLBIGMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define PMI2U_CHKLBIGMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)                \
    PMI2U_CHKLBIGMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* Persistent memory that we may want to recover if something goes wrong */
#define PMI2U_CHKPMEM_DECL(n_)                                  \
    void *(pmi2u_chkpmem_stk_[n_]) = {0};                       \
    int pmi2u_chkpmem_stk_sp_=0;                                \
    PMI2U_AssertDeclValue(const int pmi2u_chkpmem_stk_sz_,n_)
#define PMI2U_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) do {        \
        pointer_ = (type_)PMI2U_Malloc(nbytes_);                                        \
        if (pointer_) {                                                                 \
            PMI2U_Assert(pmi2u_chkpmem_stk_sp_<pmi2u_chkpmem_stk_sz_);                  \
            pmi2u_chkpmem_stk_[pmi2u_chkpmem_stk_sp_++] = pointer_;                     \
        } else {                                                                        \
            PMI2U_CHKMEM_SETERR(rc_,nbytes_,name_);                                     \
            stmt_;                                                                      \
        }                                                                               \
    } while(0)
#define PMI2U_CHKPMEM_REGISTER(pointer_) do {                           \
        PMI2U_Assert(pmi2u_chkpmem_stk_sp_<pmi2u_chkpmem_stk_sz_);      \
        pmi2u_chkpmem_stk_[pmi2u_chkpmem_stk_sp_++] = pointer_;         \
    } while(0)
#define PMI2U_CHKPMEM_REAP()                                            \
    while (pmi2u_chkpmem_stk_sp_ > 0) {                                 \
        PMI2U_Free( pmi2u_chkpmem_stk_[--pmi2u_chkpmem_stk_sp_] ); }
#define PMI2U_CHKPMEM_COMMIT() pmi2u_chkpmem_stk_sp_ = 0
#define PMI2U_CHKPMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_)          \
    PMI2U_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define PMI2U_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)           \
    PMI2U_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* A special version for routines that only allocate one item */
#define PMI2U_CHKPMEM_MALLOC1(pointer_,type_,nbytes_,rc_,name_,stmt_) do {      \
        pointer_ = (type_)PMI2U_Malloc(nbytes_);                                \
        if (!(pointer_)) {                                                      \
            PMI2U_CHKMEM_SETERR(rc_,nbytes_,name_);                             \
            stmt_;                                                              \
        }                                                                       \
    } while(0)

/* Provides a easy way to use realloc safely and avoid the temptation to use
 * realloc unsafely (direct ptr assignment).  Zero-size reallocs returning NULL
 * are handled and are not considered an error. */
#define PMI2U_REALLOC_OR_FREE_AND_JUMP(ptr_,size_,rc_) do {                                     \
        void *realloc_tmp_ = PMI2U_Realloc((ptr_), (size_));                                    \
        if ((size_) && !realloc_tmp_) {                                                         \
            PMI2U_Free(ptr_);                                                                   \
            PMI2U_ERR_SETANDJUMP2(rc_,PMI2U_CHKMEM_ISFATAL,                                     \
                                  "**nomem2","**nomem2 %d %s",(size_),PMI2U_QUOTE(ptr_));       \
        }                                                                                       \
        (ptr_) = realloc_tmp_;                                                                  \
    } while (0)
/* this version does not free ptr_ */
#define PMI2U_REALLOC_ORJUMP(ptr_,size_,rc_) do {                                               \
        void *realloc_tmp_ = PMI2U_Realloc((ptr_), (size_));                                    \
        if (size_)                                                                              \
            PMI2U_ERR_CHKANDJUMP2(!realloc_tmp_,rc_,PMI2U_CHKMEM_ISFATAL,\                      \
                                  "**nomem2","**nomem2 %d %s",(size_),PMI2U_QUOTE(ptr_));       \
        (ptr_) = realloc_tmp_;                                                                  \
    } while (0)

#endif /*SIMPLE_PMI_UTIL_H_INCLUDED*/
