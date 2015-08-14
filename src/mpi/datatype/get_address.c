/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_address */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_address = PMPI_Get_address
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_address  MPI_Get_address
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_address as PMPI_Get_address
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_address(const void *location, MPI_Aint *address) __attribute__((weak,alias("PMPI_Get_address")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_address
#define MPI_Get_address PMPI_Get_address

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_address
#undef FCNAME
#define FCNAME "MPI_Get_address"

/*@
   MPI_Get_address - Get the address of a location in memory

Input Parameters:
. location - location in caller memory (choice)

Output Parameters:
. address - address of location (address integer)

   Notes:
    This routine is provided for both the Fortran and C programmers.
    On many systems, the address returned by this routine will be the same
    as produced by the C '&' operator, but this is not required in C and
    may not be true of systems with word- rather than byte-oriented
    instructions or systems with segmented address spaces.

    This routine should be used instead of 'MPI_Address'.

.N SignalSafe

.N Fortran

 In Fortran, the integer type is always signed.  This can cause problems
 on systems where the address fits into a four byte unsigned integer but
 the value is larger than the largest signed integer.  For example, a system
 with more than 2 GBytes of memory may have addresses that do not fit within
 a four byte signed integer.  Unfortunately, there is no easy solution to
 this problem, as there is no Fortran datatype that can be used here (using
 a longer integer type will cause other problems, as well as surprising
 users when the size of the integer type is larger that the size of a pointer
 in C).  In this case, it is recommended that you use C to manipulate
 addresses.

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPI_Get_address(const void *location, MPI_Aint *address)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_ADDRESS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_ADDRESS);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(address,"address",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* SX_4 needs to set CHAR_PTR_IS_ADDRESS
       The reason is that it computes the different in two pointers in
       an "int", and addresses typically have the high (bit 31) bit set;
       thus the difference, when cast as MPI_Aint (long), is sign-extended,
       making the absolute address negative.  Without a copy of the C
       standard, I can't tell if this is a compiler bug or a language bug.
    */
#ifdef CHAR_PTR_IS_ADDRESS
    *address = MPIU_VOID_PTR_CAST_TO_MPI_AINT ((char *) location);
#else
    /* Note that this is the "portable" way to generate an address.
       The difference of two pointers is the number of elements
       between them, so this gives the number of chars between location
       and ptr.  As long as sizeof(char) represents one byte,
       of bytes from 0 to location */
    *address = MPIU_VOID_PTR_CAST_TO_MPI_AINT ((char *) location - (char *) MPI_BOTTOM);
#endif
    /* The same code is used in MPI_Address */

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ADDRESS);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_get_address",
	    "**mpi_get_address %p %p", location, address);
    }
    mpi_errno = MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
