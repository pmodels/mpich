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
 * Thread safe implementation of strerror().  The multi-threaded version 
 * will need to use thread specific storage for the string.
 * This prevents the need for allocation of heap memory each time the 
 * function is called.  Granted, stack memory could be used,
 * but allocation of large strings on the stack in a multi-threaded 
 * environment is not wise since thread stack can be relatively
 * small and a deep nesting of routines that each allocate a reasonably 
 * size error for a message can result in stack overrun.
 */
#if defined(HAVE_STRERROR)
#   if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE || USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
#       define MPIU_Strerror(errno_) strerror(errno_)
#   else
#       error need a thread safe implementation of MPIU_Strerror
#   endif
#else
#   define MPIU_Strerror(errno_) "(strerror() not found)"
#endif

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
#   define MPIU_Assert(a_)						\
    {									\
	if (!(a_))							\
	{								\
	    MPIU_Internal_error_printf("Assertion failed in file %s at line %d: %s\n", __FILE__, __LINE__, MPIU_QUOTE(a_));	\
            MPIU_DBG_MSG_FMT(ALL, TERSE, (MPIU_DBG_FDEST, "Assertion failed in file %s at line %d: %s\n", __FILE__, __LINE__, MPIU_QUOTE(a_)));	\
            MPID_Abort(NULL, MPI_SUCCESS, 1, NULL);			\
	}								\
    }
#else
#   define MPIU_Assert(a_)
/* Empty decls not allowed in C */
#   define MPIU_AssertDecl(a_) a_ 
#   define MPIU_AssertDeclValue(_a,_b) _a ATTRIBUTE((unused))
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
#define MPIU_Assertp(a_)					\
{								\
    if (!(a_))							\
    {								\
        MPIU_Internal_error_printf("Assertion failed in file %s at line %d: %s\n", __FILE__, __LINE__, MPIU_QUOTE(a_));	\
        MPIU_DBG_MSG_FMT(ALL, TERSE, (MPIU_DBG_FDEST, "Assertion failed in file %s at line %d: %s\n", __FILE__, __LINE__, MPIU_QUOTE(a_)));	\
        MPID_Abort(NULL, MPI_SUCCESS, 1, NULL);			\
    }								\
}

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

#endif /* !defined(MPIUTIL_H_INCLUDED) */
