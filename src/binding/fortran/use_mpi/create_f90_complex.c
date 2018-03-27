/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "create_f90_util.h"

#include "mpiimpl.h"
#ifdef HAVE_FC_TYPE_ROUTINES
#include "mpif90model.h"
#else
/* Assume only 4 and 8 byte IEEE reals available */
#define MPIR_F90_REAL_MODEL 6, 37
#define MPIR_F90_DOUBLE_MODEL 15, 307
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_f90_complex */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_f90_complex = PMPI_Type_create_f90_complex
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_f90_complex  MPI_Type_create_f90_complex
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_f90_complex as PMPI_Type_create_f90_complex
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_create_f90_complex(int precision, int range, MPI_Datatype * newtype)
    __attribute__ ((weak, alias("PMPI_Type_create_f90_complex")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_f90_complex PMPI_Type_create_f90_complex

#endif

typedef struct realModel {
    int digits, exponent;
    MPI_Datatype dtype;
} realModel;

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_f90_complex
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Type_create_f90_complex - Return a predefined type that matches
   the specified range

Input Parameters:
+  precision - Number of decimal digits in mantissa
-  range - Decimal range desired

Output Parameters:
.  newtype - A predefine MPI Datatype that matches the range.

   Notes:
If there is no corresponding type for the specified range, the call is
erroneous.  This implementation sets 'newtype' to 'MPI_DATATYPE_NULL' and
returns an error of class 'MPI_ERR_ARG'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Type_create_f90_complex(int precision, int range, MPI_Datatype * newtype)
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype basetype;
    static int setupPredefTypes = 1;
    static realModel f90_real_model[2] = {
        {MPIR_F90_REAL_MODEL, MPI_COMPLEX},
        {MPIR_F90_DOUBLE_MODEL, MPI_DOUBLE_COMPLEX}
    };
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_F90_COMPLEX);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_CREATE_F90_COMPLEX);

    /* ... body of routine ...  */
    /* MPI 2.1, Section 16.2, page 473 lines 12-27 make it clear that
     * these types must return the combiner version. */
    if (setupPredefTypes) {
        setupPredefTypes = 0;
        for (i = 0; i < 2; i++) {
            MPI_Datatype oldType = f90_real_model[i].dtype;
            mpi_errno = MPIR_Create_unnamed_predefined(oldType,
                                                       MPI_COMBINER_F90_COMPLEX,
                                                       f90_real_model[i].digits,
                                                       f90_real_model[i].exponent,
                                                       &f90_real_model[i].dtype);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    basetype = MPI_DATATYPE_NULL;
    for (i = 0; i < 2; i++) {
        if (f90_real_model[i].digits >= precision && f90_real_model[i].exponent >= range) {
            basetype = f90_real_model[i].dtype;
            break;
        }
    }

    if (basetype == MPI_DATATYPE_NULL) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPI_Type_create_f90_complex",
                                         __LINE__, MPI_ERR_OTHER,
                                         "**f90typecomplexnone",
                                         "**f90typecomplexnone %d %d", precision, range);
    } else {
        mpi_errno = MPIR_Create_unnamed_predefined(basetype,
                                                   MPI_COMBINER_F90_COMPLEX, range, precision,
                                                   newtype);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_CREATE_F90_COMPLEX);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_create_f90_complex",
                                 "**mpi_type_create_f90_complex %d %d", precision, range);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
