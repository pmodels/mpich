/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPIASSERT_H_INCLUDED)
#define MPIASSERT_H_INCLUDED

#include "mpiu_type_defs.h"

/* modern versions of clang support lots of C11 features */
#if defined(__has_extension)
#  if __has_extension(c_generic_selections)
#    define HAVE_C11__GENERIC 1
#  endif
#  if __has_extension(c_static_assert)
#    define HAVE_C11__STATIC_ASSERT 1
#  endif
#endif

/* GCC 4.6 added support for _Static_assert:
 * http://gcc.gnu.org/gcc-4.6/changes.html */
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined __cplusplus
#  define HAVE_C11__STATIC_ASSERT 1
#endif

/* prototypes for assertion implementation helpers */
int MPIR_Assert_fail(const char *cond, const char *file_name, int line_num);
int MPIR_Assert_fail_fmt(const char *cond, const char *file_name, int line_num, const char *fmt, ...);

/*
 * MPIU_Assert()
 *
 * Similar to assert() except that it performs an MPID_Abort() when the 
 * assertion fails.  Also, for Windows, it doesn't popup a
 * mesage box on a remote machine.
 *
 * MPIU_AssertDecl may be used to include declarations only needed
 * when MPIU_Assert is non-null (e.g., when assertions are enabled)
 */
#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))
#   define MPIU_AssertDecl(a_) a_
#   define MPIU_AssertDeclValue(_a,_b) _a = _b
#   define MPIU_Assert(a_)                             \
    do {                                               \
        if (unlikely(!(a_))) {                         \
            MPIR_Assert_fail(#a_, __FILE__, __LINE__); \
        }                                              \
    } while (0)
#else
#   define MPIU_Assert(a_)
/* Empty decls not allowed in C */
#   define MPIU_AssertDecl(a_) a_ 
#   define MPIU_AssertDeclValue(_a,_b) _a ATTRIBUTE((unused)) = _b
#endif

/*
 * MPIU_Assertp()
 *
 * Similar to MPIU_Assert() except that these assertions persist regardless of 
 * NDEBUG or HAVE_ERROR_CHECKING.  MPIU_Assertp() may
 * be used for error checking in prototype code, although it should be 
 * converted real error checking and reporting once the
 * prototype becomes part of the official and supported code base.
 */
#define MPIU_Assertp(a_)                                             \
    do {                                                             \
        if (unlikely(!(a_))) {                                       \
            MPIR_Assert_fail(#a_, __FILE__, __LINE__);               \
        }                                                            \
    } while (0)

/* Define the MPIU_Assert_fmt_msg macro.  This macro takes two arguments.  The
 * first is the condition to assert.  The second is a parenthesized list of
 * arguments suitable for passing directly to printf that will yield a relevant
 * error message.  The macro will first evaluate the condition.  If it evaluates
 * to false the macro will take four steps:
 *
 * 1) It will emit an "Assertion failed..." message in the valgrind output with
 *    a backtrace, if valgrind client requests are available and the process is
 *    running under valgrind.  It will also evaluate and print the supplied
 *    message.
 * 2) It will emit an "Assertion failed..." message via MPL_internal_error_printf.
 *    The supplied error message will also be evaluated and printed.
 * 3) It will similarly emit the assertion failure and caller supplied messages
 *    to the debug log, if enabled, via MPIU_DBG_MSG_FMT.
 * 4) It will invoke MPID_Abort, just like the other MPIU_Assert* macros.
 *
 * If the compiler doesn't support (...)/__VA_ARGS__ in macros then the user
 * message will not be evaluated or printed.  If NDEBUG is defined or
 * HAVE_ERROR_CHECKING is undefined, this macro will expand to nothing, just
 * like MPIU_Assert.
 *
 * Example usage:
 *
 * MPIU_Assert_fmg_msg(foo > bar,("foo is larger than bar: foo=%d bar=%d",foo,bar));
 */
#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))
#  if defined(HAVE_MACRO_VA_ARGS)

/* newlines are added internally by the impl function, callers do not need to include them */
#    define MPIU_Assert_fmt_msg(cond_,fmt_arg_parens_)                         \
    do {                                                                       \
        if (unlikely(!(cond_))) {                                              \
            MPIR_Assert_fail_fmt(#cond_, __FILE__, __LINE__,                   \
                                 MPIU_Assert_fmt_msg_expand_ fmt_arg_parens_); \
        }                                                                      \
    } while (0)
/* helper to just expand the parens arg inline */
#    define MPIU_Assert_fmt_msg_expand_(...) __VA_ARGS__

#  else /* defined(HAVE_MACRO_VA_ARGS) */

#    define MPIU_Assert_fmt_msg(cond_,fmt_arg_parens_)                                                   \
    do {                                                                                                 \
        if (unlikely(!(cond_))) {                                                                        \
            MPIR_Assert_fail_fmt(#cond_, __FILE__, __LINE__,                                             \
                                 "%s", "macro __VA_ARGS__ not supported, unable to print user message"); \
        }                                                                                                \
    } while (0)

#  endif
#else /* !defined(NDEBUG) && defined(HAVE_ERROR_CHECKING) */
#    define MPIU_Assert_fmt_msg(cond_,fmt_arg_parens_)
#endif

#ifdef HAVE_C11__STATIC_ASSERT
#  define MPIU_Static_assert(cond_,msg_) _Static_assert(cond_,msg_)
#endif
/* fallthrough to a run-time assertion */
#ifndef MPIU_Static_assert
#  define MPIU_Static_assert(cond_,msg_) MPIU_Assert_fmt_msg((cond_), ("%s", (msg_)))
#endif

/* evaluates to TRUE if ((a_)*(b_)>(max_)), only detects overflow for positive
 * a_ and _b. */
#define MPIU_Prod_overflows_max(a_, b_, max_) \
    ( (a_) > 0 && (b_) > 0 && ((a_) > ((max_) / (b_))) )

/* asserts that ((a_)*(b_)<=(max_)) holds in a way that is robust against
 * undefined integer overflow behavior and is suitable for both signed and
 * unsigned math (only suitable for positive values of (a_) and (b_)) */
#define MPIU_Assert_prod_pos_overflow_safe(a_, b_, max_)                               \
    MPIU_Assert_fmt_msg(!MPIU_Prod_overflows_max((a_),(b_),(max_)),                    \
                        ("overflow detected: (%llx * %llx) > %s", (a_), (b_), #max_)); \



/* -------------------------------------------------------------------------- */
/* static type checking macros */

/* implement using C11's "_Generic" functionality (optimal case) */
#ifdef HAVE_C11__GENERIC
#  define MPIU_Assert_has_type(expr_,type_) \
    MPIU_Static_assert(_Generic((expr_), type_: 1, default: 0), \
                       "expression '" #expr_ "' does not have type '" #type_ "'")
#endif
/* fallthrough to do nothing */
#ifndef MPIU_Assert_has_type
#  define MPIU_Assert_has_type(expr_,type_) do {} while (0)
#endif

#endif /* !defined(MPIASSERT_H_INCLUDED) */
