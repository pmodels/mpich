/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SIMPLE_PMIUTIL_H_INCLUDED
#define SIMPLE_PMIUTIL_H_INCLUDED

#include "mpl.h"

/* maximum sizes for arrays */
#define PMIU_MAXLINE 1024
#define PMIU_IDSIZE    32

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#define PMIU_Assert(expr) assert(expr)
#else
#define PMIU_Assert(expr)
#endif

#if defined HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#ifndef MAXHOSTNAME
#define MAXHOSTNAME 256
#endif

/* prototypes for PMIU routines */
void PMIU_Set_rank(int PMI_rank);
void PMIU_Set_rank_kvsname(int rank, const char *kvsname);
void PMIU_SetServer(void);
void PMIU_printf(int print_flag, const char *fmt, ...);
int PMIU_readline(int fd, char *buf, int max);
int PMIU_write(int fd, char *buf, int buflen);
int PMIU_writeline(int fd, char *buf);
int PMIU_parse_keyvals(char *st);
void PMIU_dump_keyvals(void);
char *PMIU_getval(const char *keystr, char *valstr, int vallen);
void PMIU_chgval(const char *keystr, char *valstr);

#define PMIU_Malloc(size_) MPL_malloc(size_, MPL_MEM_PM)
#define PMIU_Free MPL_free
#define PMIU_Strdup MPL_strdup
#define PMIU_Strnapp MPL_strnapp
#define PMIU_Exit MPL_exit
#define PMIU_Memcpy memcpy

#define PMIU_TRUE 1
#define PMIU_FALSE 0

/* We assume the following error codes are the same as PMI and PMI2 equivalent,
 * i.e. PMIU_SUCCESS == PMI_SUCCESS == PMI2_SUCCESS
 *
 * FIXME: add some mechanism to ensure this.
 */

#define PMIU_SUCCESS   0
#define PMIU_FAIL     -1
#define PMIU_ERR_NOMEM 2

extern int PMIU_verbose;        /* Set this to true to print PMI debugging info */

/* error reporting macros */
/* NOTE: we assume error codes are matching between PMI-1 and PMI-2 */
/* FIXME: do not assume error code, make it a macro parameter */

#define PMIU_ERR_POP(err) do { \
    if (err) { \
        PMIU_printf(PMIU_verbose, "ERROR: %s (%d)\n", __func__, __LINE__); \
        goto fn_fail; \
    } \
} while (0)

#define PMIU_ERR_SETANDJUMP(err, class, str) do {                              \
        PMIU_printf(PMIU_verbose, "ERROR: "str" in %s (%d)\n", __func__, __LINE__);     \
        err = class;                                                      \
        goto fn_fail;                                                           \
    } while (0)
#define PMIU_ERR_SETANDJUMP1(err, class, str, arg) do {                          \
        PMIU_printf(PMIU_verbose, "ERROR: "str" in %s (%d)\n", arg, __func__, __LINE__);       \
        err = class;                                                              \
        goto fn_fail;                                                                   \
    } while (0)
#define PMIU_ERR_SETANDJUMP2(err, class, str, arg1, arg2) do {                           \
        PMIU_printf(PMIU_verbose, "ERROR: "str" in %s (%d)\n", arg1, arg2, __func__, __LINE__);        \
        err = class;                                                                      \
        goto fn_fail;                                                                           \
    } while (0)
#define PMIU_ERR_SETANDJUMP3(err, class, str, arg1, arg2, arg3) do {                     \
        PMIU_printf(PMIU_verbose, "ERROR: "str" in %s (%d)\n", arg1, arg2, arg3, __func__, __LINE__);  \
        err = class;                                                                      \
        goto fn_fail;                                                                           \
    } while (0)
#define PMIU_ERR_SETANDJUMP4(err, class, str, arg1, arg2, arg3, arg4) do {                       \
        PMIU_printf(PMIU_verbose, "ERROR: "str" in %s (%d)\n", arg1, arg2, arg3, arg4, __func__, __LINE__);    \
        err = class;                                                                              \
        goto fn_fail;                                                                                   \
    } while (0)
#define PMIU_ERR_SETANDJUMP5(err, class, str, arg1, arg2, arg3, arg4, arg5) do {                         \
        PMIU_printf(PMIU_verbose, "ERROR: "str" in %s (%d)\n", arg1, arg2, arg3, arg4, arg5, __func__, __LINE__);      \
        err = class;                                                                                      \
        goto fn_fail;                                                                                           \
    } while (0)
#define PMIU_ERR_SETANDJUMP6(err, class, str, arg1, arg2, arg3, arg4, arg5, arg6) do {                           \
        PMIU_printf(PMIU_verbose, "ERROR: "str" in %s (%d)\n", arg1, arg2, arg3, arg4, arg5, arg6, __func__, __LINE__);        \
        err = class;                                                                                              \
        goto fn_fail;                                                                                                   \
    } while (0)


#define PMIU_ERR_CHKANDJUMP(cond, err, class, str) do {        \
        if (cond)                                               \
            PMIU_ERR_SETANDJUMP(err, class, str);              \
    } while (0)
#define PMIU_ERR_CHKANDJUMP1(cond, err, class, str, arg) do {    \
        if (cond)                                                       \
            PMIU_ERR_SETANDJUMP1(err, class, str, arg);          \
    } while (0)
#define PMIU_ERR_CHKANDJUMP2(cond, err, class, str, arg1, arg2) do {     \
        if (cond)                                                               \
            PMIU_ERR_SETANDJUMP2(err, class, str, arg1, arg2);           \
    } while (0)
#define PMIU_ERR_CHKANDJUMP3(cond, err, class, str, arg1, arg2, arg3) do {       \
        if (cond)                                                                       \
            PMIU_ERR_SETANDJUMP3(err, class, str, arg1, arg2, arg3);             \
    } while (0)
#define PMIU_ERR_CHKANDJUMP4(cond, err, class, str, arg1, arg2, arg3, arg4) do { \
        if (cond)                                                                       \
            PMIU_ERR_SETANDJUMP4(err, class, str, arg1, arg2, arg3, arg4);       \
    } while (0)
#define PMIU_ERR_CHKANDJUMP5(cond, err, class, str, arg1, arg2, arg3, arg4, arg5) do {   \
        if (cond)                                                                               \
            PMIU_ERR_SETANDJUMP5(err, class, str, arg1, arg2, arg3, arg4, arg5);         \
    } while (0)
#define PMIU_ERR_CHKANDJUMP6(cond, err, class, str, arg1, arg2, arg3, arg4, arg5, arg6) do {     \
        if (cond)                                                                                       \
            PMIU_ERR_SETANDJUMP6(err, class, str, arg1, arg2, arg3, arg4, arg5, arg6);           \
    } while (0)

#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))
#define PMIU_AssertDecl(a_) a_
#define PMIU_AssertDeclValue(_a, _b) _a = _b
#else
/* Empty decls not allowed in C */
#define PMIU_AssertDecl(a_) a_
#define PMIU_AssertDeclValue(_a, _b) _a ATTRIBUTE((unused))
#endif

#ifdef HAVE_ERROR_CHECKING
#define PMIU_CHKMEM_SETERR(rc_, errcode, nbytes_, name_) do { \
        rc_ = errcode; \
        PMIU_printf(PMIU_verbose, "ERROR: memory allocation of %lu bytes failed for %s in %s (%d)\n",     \
                    (size_t)nbytes_, name_, __func__, __LINE__);                                  \
    } while (0)
#else
#define PMIU_CHKMEM_SETERR(rc_, errcode, nbytes_, name_) do { \
        rc_ = errcode; \
    } while (0)
#endif

#define PMIU_CHK_MALLOC(pointer_,type_,nbytes_,rc_,errcode,name_) do { \
        pointer_ = (type_)PMIU_Malloc(nbytes_); \
        if (!pointer_) { \
            PMIU_CHKMEM_SETERR(rc_,errcode,nbytes_,name_); \
            goto fn_fail; \
        } \
    } while (0)

/* Provides a easy way to use realloc safely and avoid the temptation to use
 * realloc unsafely (direct ptr assignment).  Zero-size reallocs returning NULL
 * are handled and are not considered an error. */
#define PMIU_REALLOC_OR_FREE_AND_JUMP(ptr_,size_,rc_) do {                                     \
        void *realloc_tmp_ = PMIU_Realloc((ptr_), (size_));                                    \
        if ((size_) && !realloc_tmp_) {                                                         \
            PMIU_Free(ptr_);                                                                   \
            PMIU_ERR_SETANDJUMP2(rc_,PMIU_CHKMEM_ISFATAL,                                     \
                                  "**nomem2","**nomem2 %d %s",(size_),PMIU_QUOTE(ptr_));       \
        }                                                                                       \
        (ptr_) = realloc_tmp_;                                                                  \
    } while (0)
/* this version does not free ptr_ */
#define PMIU_REALLOC_ORJUMP(ptr_,size_,rc_) do {                                               \
        void *realloc_tmp_ = PMIU_Realloc((ptr_), (size_));                                    \
        if (size_)                                                                              \
            PMIU_ERR_CHKANDJUMP2(!realloc_tmp_,rc_,PMIU_CHKMEM_ISFATAL,\                      \
                                  "**nomem2","**nomem2 %d %s",(size_),PMIU_QUOTE(ptr_));       \
        (ptr_) = realloc_tmp_;                                                                  \
    } while (0)

extern int PMIU_is_threaded;
extern MPL_thread_mutex_t PMIU_mutex;

void PMIU_thread_init(void);
void PMIU_cs_enter(void);
void PMIU_cs_exit(void);

#define PMIU_CS_ENTER do { \
    if (PMIU_is_threaded) { \
        int err; \
        MPL_thread_mutex_lock(&PMIU_mutex, &err, MPL_THREAD_PRIO_HIGH); \
        PMIU_Assert(err == 0); \
    } \
} while (0)

#define PMIU_CS_EXIT do { \
    if (PMIU_is_threaded) { \
        int err; \
        MPL_thread_mutex_unlock(&PMIU_mutex, &err); \
        PMIU_Assert(err == 0); \
    } \
} while (0)

#endif /* SIMPLE_PMIUTIL_H_INCLUDED */
