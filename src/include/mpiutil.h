/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPIUTIL_H_INCLUDED)
#define MPIUTIL_H_INCLUDED

#ifndef HAS_MPID_ABORT_DECL
/* FIXME: 4th arg is undocumented and bogus */
struct MPID_Comm;
int MPID_Abort( struct MPID_Comm *comm, int mpi_errno, int exit_code, const char *error_msg );
#endif

/*
 * MPIU_Sterror()
 *
 * Thread safe implementation of strerror(), whenever possible. */
const char *MPIU_Strerror(int errnum);

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
        if (!(a_)) {                                   \
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
        if (!(a_)) {                                                 \
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
 * 2) It will emit an "Assertion failed..." message via MPIU_Internal_error_printf.
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
        if (!(cond_)) {                                                        \
            MPIR_Assert_fail_fmt(#cond_, __FILE__, __LINE__,                   \
                                 MPIU_Assert_fmt_msg_expand_ fmt_arg_parens_); \
        }                                                                      \
    } while (0)
/* helper to just expand the parens arg inline */
#    define MPIU_Assert_fmt_msg_expand_(...) __VA_ARGS__

#  else /* defined(HAVE_MACRO_VA_ARGS) */

#    define MPIU_Assert_fmt_msg(cond_,fmt_arg_parens_)                                                   \
    do {                                                                                                 \
        if (!(cond_)) {                                                                                  \
            MPIR_Assert_fail_fmt(#cond_, __FILE__, __LINE__,                                             \
                                 "%s", "macro __VA_ARGS__ not supported, unable to print user message"); \
        }                                                                                                \
    } while (0)

#  endif
#else /* !defined(NDEBUG) && defined(HAVE_ERROR_CHECKING) */
#    define MPIU_Assert_fmt_msg(cond_,fmt_arg_parens_)
#endif

/*
 * Ensure an MPI_Aint value fits into a signed int.
 * Useful for detecting overflow when MPI_Aint is larger than an int.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#define MPID_Ensure_Aint_fits_in_int(aint) \
  MPIU_Assert((aint) == (MPI_Aint)(int)(aint));

/*
 * Ensure an MPI_Aint value fits into an unsigned int.
 * Useful for detecting overflow when MPI_Aint is larger than an 
 * unsigned int.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#define MPID_Ensure_Aint_fits_in_uint(aint) \
  MPIU_Assert((aint) == (MPI_Aint)(unsigned int)(aint));

/*
 * Ensure an MPI_Aint value fits into a pointer.
 * Useful for detecting overflow when MPI_Aint is larger than a pointer.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#define MPID_Ensure_Aint_fits_in_pointer(aint) \
  MPIU_Assert((aint) == (MPI_Aint)(MPIR_Upint) MPI_AINT_CAST_TO_VOID_PTR(aint));

/*
 * Basic utility macros
 */
#define MPIU_QUOTE(A) MPIU_QUOTE2(A)
#define MPIU_QUOTE2(A) #A

/* This macro is used to silence warnings from the Mac OS X linker when
 * an object file "has no symbols".  The unused attribute prevents a
 * warning about the unused dummy variable while the used attribute
 * prevents the compiler from discarding the symbol altogether.  This
 * macro should be used at the top of any file that might not define any
 * other variables or functions (perhaps due to conditional compilation
 * via the preprocessor).  A semicolon is expected after each invocation
 * of this macro. */
#define MPIU_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING \
    static int MPIU_UNIQUE_SYMBOL_NAME(dummy) ATTRIBUTE((unused,used)) = 0

/* we jump through a couple of extra macro hoops to append the line
 * number to the variable name in order to reduce the chance of a name
 * collision with headers that might be included in the translation
 * unit */
#define MPIU_UNIQUE_SYMBOL_NAME(prefix_) MPIU_UNIQUE_IMPL1_(prefix_##_unique_L,__LINE__)
#define MPIU_UNIQUE_IMPL1_(prefix_,line_) MPIU_UNIQUE_IMPL2_(prefix_,line_)
#define MPIU_UNIQUE_IMPL2_(prefix_,line_) MPIU_UNIQUE_IMPL3_(prefix_,line_)
#define MPIU_UNIQUE_IMPL3_(prefix_,line_) prefix_##line_

/* These likely/unlikely macros provide static branch prediction hints to the
 * compiler, if such hints are available.  Simply wrap the relevant expression in
 * the macro, like this:
 *
 * if (unlikely(ptr == NULL)) {
 *     // ... some unlikely code path ...
 * }
 *
 * They should be used sparingly, especially in upper-level code.  It's easy to
 * incorrectly estimate branching likelihood, while the compiler can often do a
 * decent job if left to its own devices.
 *
 * These macros are not namespaced because the namespacing is cumbersome.
 */
/* safety guard for now, add a configure check in the future */
#if defined(__GNUC__) && (__GNUC__ >= 3)
#  define unlikely(x_) __builtin_expect(!!(x_),0)
#  define likely(x_)   __builtin_expect(!!(x_),1)
#else
#  define unlikely(x_) (x_)
#  define likely(x_)   (x_)
#endif

#endif /* !defined(MPIUTIL_H_INCLUDED) */
