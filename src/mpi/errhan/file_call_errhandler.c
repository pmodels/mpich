/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* mpiext.h contains the prototypes for functions to interface MPICH
   and ROMIO */
#include "mpiext.h"

/* -- Begin Profiling Symbol Block for routine MPI_File_call_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_call_errhandler = PMPI_File_call_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_call_errhandler  MPI_File_call_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_call_errhandler as PMPI_File_call_errhandler
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_call_errhandler(MPI_File fh, int errorcode) __attribute__((weak,alias("PMPI_File_call_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_File_call_errhandler
#define MPI_File_call_errhandler PMPI_File_call_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_File_call_errhandler
#undef FCNAME
#define FCNAME "MPI_File_call_errhander"
/*@
   MPI_File_call_errhandler - Call the error handler installed on a 
   file

Input Parameters:
+ fh - MPI file with error handler (handle) 
- errorcode - error code (integer) 

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_FILE
@*/
int MPI_File_call_errhandler(MPI_File fh, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef MPI_MODE_RDONLY
    MPID_Errhandler *e;
    MPI_Errhandler eh;
#endif
    MPID_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    MPID_THREADPRIV_GET;

#ifdef MPI_MODE_RDONLY
    /* Validate parameters, especially handles needing to be converted */
    /* FIXME: check for a valid file handle (fh) before converting to a 
       pointer */
    
    /* ... body of routine ...  */
    
    MPIR_ROMIO_Get_file_errhand( fh, &eh );
    /* Check for the special case of errors-throw-exception.  In this case
       return the error code; the C++ wrapper will cause an exception to
       be thrown.
       */
#ifdef HAVE_CXX_BINDING
    if (eh == MPIR_ERRORS_THROW_EXCEPTIONS) {
	mpi_errno = errorcode;
	goto fn_exit;
    }
#endif
    if (!eh) {
	MPID_Errhandler_get_ptr( MPI_ERRORS_RETURN, e );
    }
    else {
	MPID_Errhandler_get_ptr( eh, e );
    }

    /* Note that, unlike the rest of MPICH, MPI_File objects are pointers,
       not integers.  */

    if (e->handle == MPI_ERRORS_RETURN) {
	goto fn_exit;
    }

    if (e->handle == MPI_ERRORS_ARE_FATAL) {
	MPIR_Handle_fatal_error(NULL, "MPI_File_call_errhandler", errorcode);
    }

    switch (e->language) {
    case MPID_LANG_C:
	(*e->errfn.C_File_Handler_function)( &fh, &errorcode );
	break;
#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX:
	/* See HAVE_LANGUAGE_FORTRAN below for an explanation */
    { void *fh1 = (void *)&fh;
	(*MPIR_Process.cxx_call_errfn)( 1, fh1, &errorcode, 
			(void (*)(void))*e->errfn.C_File_Handler_function );
	}
	break;
#endif
#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	/* The assignemt to a local variable prevents the compiler
	   from generating a warning about a type-punned pointer.  Since
	   the value is really const (but MPI didn't define error handlers 
	   with const), this preserves the intent */
	{ void *fh1 = (void *)&fh;
	    MPI_Fint ferr = errorcode;  /* Needed if MPI_Fint and int aren't 
					   the same size */
	(*e->errfn.F77_Handler_function)( fh1, &ferr );
	}
	break;
#endif
    }

#else
    /* Dummy in case ROMIO is not defined */
    mpi_errno = MPI_ERR_INTERN;
#endif
    /* ... end of body of routine ... */

#if defined(HAVE_CXX_BINDING) && defined(MPI_MODE_RDONLY)
 fn_exit:
#else 
 fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
    return mpi_errno;
}

#ifndef MPICH_MPI_FROM_PMPI
/* This is a glue routine that can be used by ROMIO 
   (see mpi-io/glue/mpich/mpio_err.c) to properly invoke the C++ 
   error handler */
#undef FUNCNAME
#define FUNCNAME MPIR_File_call_cxx_errhandler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_File_call_cxx_errhandler( MPI_File *fh, int *errorcode, 
			   void (*c_errhandler)(MPI_File *, int *, ... ) )
{
    /* ROMIO will contain a reference to this routine, so if there is 
       no C++ support, it will never be called but it must be availavle. */
#ifdef HAVE_CXX_BINDING
    void *fh1 = (void *)fh;
    (*MPIR_Process.cxx_call_errfn)( 1, fh1, errorcode, (void(*)(void))c_errhandler );
    /* The C++ code throws an exception if the error handler 
       returns something other than MPI_SUCCESS. There is no "return"
       of an error code. This code mirrors that in errutil.c */
    *errorcode = MPI_SUCCESS;
#endif
    return *errorcode;
}
#endif
