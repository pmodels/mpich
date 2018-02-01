/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_ASSERT_H_INCLUDED
#define MPIR_ASSERT_H_INCLUDED

#include "mpir_type_defs.h"

/* modern versions of clang support lots of C11 features */
#if defined(__has_extension)
#if __has_extension(c_generic_selections)
#define HAVE_C11__GENERIC 1
#endif
#if __has_extension(c_static_assert)
#define HAVE_C11__STATIC_ASSERT 1
#endif
#endif

/* GCC 4.6 added support for _Static_assert:
 * http://gcc.gnu.org/gcc-4.6/changes.html */
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined __cplusplus
#define HAVE_C11__STATIC_ASSERT 1
#endif

/* prototypes for assertion implementation helpers */
int MPIR_Assert_fail(const char *cond, const char *file_name, int line_num);
int MPIR_Assert_fail_fmt(const char *cond, const char *file_name, int line_num, const char *fmt,
                         ...);

/*
 * MPIR_Assert()
 *
 * Similar to assert() except that it performs an MPID_Abort() when the
 * assertion fails.  Also, for Windows, it doesn't popup a
 * mesage box on a remote machine.
 */
#if (defined(__COVERITY__) || defined(__KLOCWORK__))
#include <assert.h>
#define MPIR_Assert(a_) assert(a_);
#elif (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))
#define MPIR_AssertDeclValue(_a,_b) _a = _b
#define MPIR_Assert(a_)                             \
    do {                                               \
        if (unlikely(!(a_))) {                         \
            MPIR_Assert_fail(#a_, __FILE__, __LINE__); \
        }                                              \
    } while (0)
#else
#define MPIR_Assert(a_)
/* Empty decls not allowed in C */
#define MPIR_AssertDeclValue(_a,_b) _a ATTRIBUTE((unused)) = _b
#endif

/*
 * MPIR_Assertp()
 *
 * Similar to MPIR_Assert() except that these assertions persist regardless of
 * NDEBUG or HAVE_ERROR_CHECKING.  MPIR_Assertp() may
 * be used for error checking in prototype code, although it should be
 * converted real error checking and reporting once the
 * prototype becomes part of the official and supported code base.
 */
#define MPIR_Assertp(a_)                                             \
    do {                                                             \
        if (unlikely(!(a_))) {                                       \
            MPIR_Assert_fail(#a_, __FILE__, __LINE__);               \
        }                                                            \
    } while (0)

/* Define the MPIR_Assert_fmt_msg macro.  This macro takes two arguments.  The
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
 *    to the debug log, if enabled, via MPL_DBG_MSG_FMT.
 * 4) It will invoke MPID_Abort, just like the other MPIR_Assert* macros.
 *
 * If the compiler doesn't support (...)/__VA_ARGS__ in macros then the user
 * message will not be evaluated or printed.  If NDEBUG is defined or
 * HAVE_ERROR_CHECKING is undefined, this macro will expand to nothing, just
 * like MPIR_Assert.
 *
 * Example usage:
 *
 * MPIR_Assert_fmg_msg(foo > bar,("foo is larger than bar: foo=%d bar=%d",foo,bar));
 */
#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))
#if defined(HAVE_MACRO_VA_ARGS)

/* newlines are added internally by the impl function, callers do not need to include them */
#define MPIR_Assert_fmt_msg(cond_,fmt_arg_parens_)                         \
    do {                                                                       \
        if (unlikely(!(cond_))) {                                              \
            MPIR_Assert_fail_fmt(#cond_, __FILE__, __LINE__,                   \
                                 fmt_msg_expand_ fmt_arg_parens_); \
        }                                                                      \
    } while (0)
/* helper to just expand the parens arg inline */
#define fmt_msg_expand_(...) __VA_ARGS__

#else /* defined(HAVE_MACRO_VA_ARGS) */

#define MPIR_Assert_fmt_msg(cond_,fmt_arg_parens_)                                                   \
    do {                                                                                                 \
        if (unlikely(!(cond_))) {                                                                        \
            MPIR_Assert_fail_fmt(#cond_, __FILE__, __LINE__,                                             \
                                 "%s", "macro __VA_ARGS__ not supported, unable to print user message"); \
        }                                                                                                \
    } while (0)

#endif
#else /* !defined(NDEBUG) && defined(HAVE_ERROR_CHECKING) */
#define MPIR_Assert_fmt_msg(cond_,fmt_arg_parens_)
#endif

#ifdef HAVE_C11__STATIC_ASSERT
#define MPIR_Static_assert(cond_,msg_) _Static_assert(cond_,msg_)
#endif
/* fallthrough to a run-time assertion */
#ifndef MPIR_Static_assert
#define MPIR_Static_assert(cond_,msg_) MPIR_Assert_fmt_msg((cond_), ("%s", (msg_)))
#endif

#endif /* MPIR_ASSERT_H_INCLUDED */
