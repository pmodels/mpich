/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdarg.h>
#include <armciconf.h>

enum debug_cats_e {
  DEBUG_CAT_ALL        =  -1,
  DEBUG_CAT_NONE       =   0,
  DEBUG_CAT_MEM_REGION = 0x1,  // 2^0
  DEBUG_CAT_ALLOC      = 0x2,  // 2^1
  DEBUG_CAT_MUTEX      = 0x4,  // 2^2
  DEBUG_CAT_GROUPS     = 0x8,  // 2^3
  DEBUG_CAT_CTREE      = 0x10, // 2^4, ...
  DEBUG_CAT_IOV        = 0x20
};

/* A logical OR of the debug message categories that are enabled.
 */
extern  unsigned DEBUG_CATS_ENABLED;


#ifdef NO_SEATBELTS
#define ARMCII_Assert(X) ((void)0)
#define ARMCII_Assert_msg(X,MSG) ((void)0)
#else
void    ARMCII_Assert_fail(const char *expr, const char *msg, const char *file, int line, const char *func);
#define ARMCII_Assert(EXPR)          do { if (!(EXPR)) ARMCII_Assert_fail(#EXPR, NULL, __FILE__, __LINE__, __func__); } while(0)
#define ARMCII_Assert_msg(EXPR, MSG) do { if (!(EXPR)) ARMCII_Assert_fail(#EXPR, MSG,  __FILE__, __LINE__, __func__); } while(0)
#endif /* NO_SEATBELTS    */


#ifdef NO_SEATBELTS
#define DEBUG_CAT_ENABLED(X) 0
#define ARMCII_Dbg_print(CAT,...) ((void)0)
#else
#define DEBUG_CAT_ENABLED(X) (DEBUG_CATS_ENABLED & (X))
void    ARMCII_Dbg_print_impl(const char *func, const char *format, ...);
#define ARMCII_Dbg_print(CAT,...) do { if (DEBUG_CAT_ENABLED(CAT)) ARMCII_Dbg_print_impl(__func__,__VA_ARGS__); } while (0)
#endif /* NO_SEATBELTS */


#define ARMCII_Error(...) ARMCII_Error_impl(__FILE__,__LINE__,__func__,__VA_ARGS__)
void    ARMCII_Error_impl(const char *file, const int line, const char *func, const char *msg, ...);
void    ARMCII_Warning(const char *fmt, ...);

#endif /* _DEBUG_H_ */
