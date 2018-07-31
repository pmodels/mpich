/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_ERR_H_INCLUDED
#define HYDRA_ERR_H_INCLUDED

#include "hydra_base.h"

#define DBG_PREFIX_LEN  (256)
extern char HYD_print_prefix_str[DBG_PREFIX_LEN];

#define PRINT_PREFIX(fp)                                \
    {                                                   \
        fprintf(fp, "[%s] ", HYD_print_prefix_str);     \
        fflush(fp);                                     \
    }

#define HYD_PRINT_NOPREFIX(fp, ...)             \
    {                                           \
        fprintf(fp, __VA_ARGS__);               \
        fflush(fp);                             \
    }

#define HYD_PRINT(fp, ...)                      \
    {                                           \
        PRINT_PREFIX(fp);                       \
        HYD_PRINT_NOPREFIX(fp, __VA_ARGS__);    \
    }

#if defined HAVE__FUNC__
#define FUNC __func__
#elif defined HAVE_CAP__FUNC__
#define FUNC __FUNC__
#elif defined HAVE__FUNCTION__
#define FUNC __FUNCTION__
#endif

#if defined __FILE__ && defined FUNC
#define HYD_ERR_PRINT(...)                                              \
    {                                                                   \
        PRINT_PREFIX(stderr);                                           \
        HYD_PRINT_NOPREFIX(stderr, "%s (%s:%d): ", FUNC, __FILE__, __LINE__); \
        HYD_PRINT_NOPREFIX(stderr, __VA_ARGS__);                        \
    }
#elif defined __FILE__
#define HYD_ERR_PRINT(...)                                              \
    {                                                                   \
        PRINT_PREFIX(stderr);                                           \
        HYD_PRINT_NOPREFIX(stderr, "%s (%d): ", __FILE__, __LINE__);    \
        HYD_PRINT_NOPREFIX(stderr, __VA_ARGS__);                        \
    }
#else
#define HYD_ERR_PRINT(...)                              \
    {                                                   \
        PRINT_PREFIX(stderr);                           \
        HYD_PRINT_NOPREFIX(stderr, __VA_ARGS__);        \
    }
#endif

#define HYD_ASSERT(x, status)                                   \
    {                                                           \
        if (!(x)) {                                             \
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL,        \
                               "assert (%s) failed\n", #x);     \
        }                                                       \
    }

#define HYD_ERR_POP(status, ...)                \
    {                                           \
        if (status) {                           \
            HYD_ERR_PRINT(__VA_ARGS__);         \
            goto fn_fail;                       \
        }                                       \
    }

#define HYD_ERR_SETANDJUMP(status, error, ...)  \
    {                                           \
        status = error;                         \
        HYD_ERR_POP(status, __VA_ARGS__);       \
    }

#define HYD_ERR_CHKANDJUMP(status, chk, error, ...)             \
    {                                                           \
        if ((chk))                                              \
            HYD_ERR_SETANDJUMP(status, error, __VA_ARGS__);     \
    }

#if defined HAVE_HERROR
#define HYD_herror herror
#else
#define HYD_herror(x) "<<<herror unavailable: cannot convert error code to string>>>"
#endif /* HAVE_HERROR */

HYD_status HYD_print_set_prefix_str(const char *str);

#endif /* HYDRA_ERR_H_INCLUDED */
