/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMI2_UTIL_H_INCLUDED
#define PMI2_UTIL_H_INCLUDED

extern int PMI2_pmiverbose;     /* Set this to true to print PMI debugging info */
#define printf_d(x...)  do { if (PMI2_pmiverbose) printf(x); } while (0)

/* error reporting macros */

#define PMI2U_ERR_POP(err) do { pmi2_errno = err; printf_d("ERROR: %s (%d)\n", __func__, __LINE__); goto fn_fail; } while (0)
#define PMI2U_ERR_SETANDJUMP(err, class, str) do {                              \
        printf_d("ERROR: "str" in %s (%d)\n", __func__, __LINE__);     \
        pmi2_errno = class;                                                      \
        goto fn_fail;                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP1(err, class, str, str1, arg) do {                          \
        printf_d("ERROR: "str1" in %s (%d)\n", arg, __func__, __LINE__);       \
        pmi2_errno = class;                                                              \
        goto fn_fail;                                                                   \
    } while (0)
#define PMI2U_ERR_SETANDJUMP2(err, class, str, str1, arg1, arg2) do {                           \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, __func__, __LINE__);        \
        pmi2_errno = class;                                                                      \
        goto fn_fail;                                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP3(err, class, str, str1, arg1, arg2, arg3) do {                     \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, __func__, __LINE__);  \
        pmi2_errno = class;                                                                      \
        goto fn_fail;                                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP4(err, class, str, str1, arg1, arg2, arg3, arg4) do {                       \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, arg4, __func__, __LINE__);    \
        pmi2_errno = class;                                                                              \
        goto fn_fail;                                                                                   \
    } while (0)
#define PMI2U_ERR_SETANDJUMP5(err, class, str, str1, arg1, arg2, arg3, arg4, arg5) do {                         \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, arg4, arg5, __func__, __LINE__);      \
        pmi2_errno = class;                                                                                      \
        goto fn_fail;                                                                                           \
    } while (0)
#define PMI2U_ERR_SETANDJUMP6(err, class, str, str1, arg1, arg2, arg3, arg4, arg5, arg6) do {                           \
        printf_d("ERROR: "str1" in %s (%d)\n", arg1, arg2, arg3, arg4, arg5, arg6, __func__, __LINE__);        \
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
                (size_t)nbytes_, name_, __func__, __LINE__);                                  \
    } while (0)
#else
#define PMI2U_CHKMEM_SETERR(rc_, nbytes_, name_) rc_ = PMI2_ERR_NOMEM
#endif

#define PMI2U_CHK_MALLOC(pointer_,type_,nbytes_,rc_,name_) do { \
        pointer_ = (type_)PMI2U_Malloc(nbytes_); \
        if (!pointer_) { \
            PMI2U_CHKMEM_SETERR(rc_,nbytes_,name_); \
            goto fn_fail; \
        } \
    } while (0)

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

#endif /* PMI2_UTIL_H_INCLUDED */
