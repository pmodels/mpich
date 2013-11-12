/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPIUTIL_H_INCLUDED)
#define MPIUTIL_H_INCLUDED

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_NEMESIS_POLLS_BEFORE_YIELD
      category    : NEMESIS
      type        : int
      default     : 1000
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        When MPICH is in a busy waiting loop, it will periodically
        call a function to yield the processor.  This cvar sets
        the number of loops before the yield function is called.  A
        value of 0 disables yielding.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#ifndef HAS_MPID_ABORT_DECL
/* FIXME: 4th arg is undocumented and bogus */
struct MPID_Comm;
int MPID_Abort( struct MPID_Comm *comm, int mpi_errno, int exit_code, const char *error_msg );
#endif

/* -------------------------------------------------------------------------- */
/* detect compiler characteristics from predefined preprocesor tokens */

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

/* -------------------------------------------------------------------------- */

/*
 * MPIU_Sterror()
 *
 * Thread safe implementation of strerror(), whenever possible. */
const char *MPIU_Strerror(int errnum);

/*
 * MPIU_Busy_wait()
 *
 * Call this in every busy wait loop to periodically yield the processor.  The
 * MPIR_CVAR_NEMESIS_POLLS_BEFORE_YIELD parameter can be used to adjust the number of
 * times MPIU_Busy_wait is called before the yield function is called.
 */
#ifdef USE_NOTHING_FOR_YIELD
/* optimize if we're not yielding the processor */
#define MPIU_Busy_wait() do {} while (0)
#else
/* MT: Updating the static int poll_count variable isn't thread safe and will
   need to be changed for fine-grained multithreading.  A possible alternative
   is to make it a global thread-local variable. */
#define MPIU_Busy_wait() do {                                   \
        if (MPIR_CVAR_NEMESIS_POLLS_BEFORE_YIELD) {                    \
            static int poll_count_ = 0;                         \
            if (poll_count_ >= MPIR_CVAR_NEMESIS_POLLS_BEFORE_YIELD) { \
                poll_count_ = 0;                                \
                MPIU_PW_Sched_yield();                          \
            } else {                                            \
                ++poll_count_;                                  \
            }                                                   \
        }                                                       \
    } while (0)
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


/* Helper macros that give us a crude version of C++'s
 * "std::numeric_limits<TYPE>" functionality.  These rely on either C11
 * "_Generic" functionality or some unfortunately complicated GCC builtins. */
#if defined(HAVE_C11__GENERIC)
#define expr_inttype_max(expr_)              \
    _Generic(expr_,                          \
             signed char:        SCHAR_MAX,  \
             signed short:       SHRT_MAX,   \
             signed int:         INT_MAX,    \
             signed long:        LONG_MAX,   \
             signed long long:   LLONG_MAX,  \
             unsigned char:      UCHAR_MAX,  \
             unsigned short:     USHRT_MAX,  \
             unsigned int:       UINT_MAX,   \
             unsigned long:      ULONG_MAX,  \
             unsigned long long: ULLONG_MAX,\
             /* _Generic cares about cv-qualifiers */ \
             volatile signed int:   INT_MAX,          \
             volatile unsigned int: UINT_MAX)
#define expr_inttype_min(expr_)             \
    _Generic(expr_,                         \
             signed char:        SCHAR_MIN, \
             signed short:       SHRT_MIN,  \
             signed int:         INT_MIN,   \
             signed long:        LONG_MIN,  \
             signed long long:   LLONG_MIN, \
             unsigned char:      0,         \
             unsigned short:     0,         \
             unsigned int:       0,         \
             unsigned long:      0,         \
             unsigned long long: 0,         \
             /* _Generic cares about cv-qualifiers */ \
             volatile signed int:   INT_MIN,          \
             volatile unsigned int: 0)
#endif

/* Assigns (src_) to (dst_), checking that (src_) fits in (dst_) without
 * truncation.
 *
 * When fiddling with this macro, please keep C's overly complicated integer
 * promotion/truncation/conversion rules in mind.  A discussion of these issues
 * can be found in Chapter 5 of "Secure Coding in C and C++" by Robert Seacord.
 */
/* this "check for overflow" macro seems buggy in a crazy way that I can't
 * figure out. Instead of using the clever 'expr_inttype_max' macro, fall back
 * to simple "cast and check for obvious overflow" */
#if 0
#if defined(expr_inttype_max) && defined(expr_inttype_min)
#  define MPIU_Assign_trunc(dst_,src_,dst_type_)                                       \
    do {                                                                               \
        MPIU_Assert_has_type((dst_), dst_type_);                                       \
        MPIU_Assert((src_) <= expr_inttype_max(dst_));                                 \
        MPIU_Assert((src_) >= expr_inttype_min(dst_));                                 \
        dst_ = (dst_type_)(src_);                                                      \
    } while (0)
#endif
#endif
#define MPIU_Assign_trunc(dst_,src_,dst_type_)                                         \
    do {                                                                               \
        /* will catch some of the cases if the expr_inttype macros aren't available */ \
        MPIU_Assert((src_) == (dst_type_)(src_));                                      \
        dst_ = (dst_type_)(src_);                                                      \
    } while (0)

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

/* -------------------------------------------------------------------------- */
/* static assertions
 *
 * C11 adds static assertions.  They can be faked in pre-C11 compilers through
 * various tricks as long as you are willing to accept some ugly error messages
 * when the assert trips.  See
 * http://www.pixelbeat.org/programming/gcc/static_assert.html
 * for some implementation options.
 *
 * For now, we just use some simple preprocessor tests to use real static
 * assertions when the compiler supports them (modern clang and gcc do).
 * Eventually add configure logic to detect this.
 */
/* the case we usually want to hit */
#ifdef HAVE_C11__STATIC_ASSERT
#  define MPIU_Static_assert(cond_,msg_) _Static_assert(cond_,msg_)
#endif
/* fallthrough to a run-time assertion */
#ifndef MPIU_Static_assert
#  define MPIU_Static_assert(cond_,msg_) MPIU_Assert_fmt_msg((cond_), ("%s", (msg_)))
#endif

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

/* -------------------------------------------------------------------------- */
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
#ifdef HAVE_BUILTIN_EXPECT
#  define unlikely(x_) __builtin_expect(!!(x_),0)
#  define likely(x_)   __builtin_expect(!!(x_),1)
#else
#  define unlikely(x_) (x_)
#  define likely(x_)   (x_)
#endif

#endif /* !defined(MPIUTIL_H_INCLUDED) */
