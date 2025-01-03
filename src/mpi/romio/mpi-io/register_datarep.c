/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include "adio_extern.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Register_datarep = PMPI_Register_datarep
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Register_datarep MPI_Register_datarep
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Register_datarep as PMPI_Register_datarep
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Register_datarep(const char *datarep, MPI_Datarep_conversion_function * read_conversion_fn,
                         MPI_Datarep_conversion_function * write_conversion_fn,
                         MPI_Datarep_extent_function * dtype_file_extent_fn, void *extra_state)
    __attribute__ ((weak, alias("PMPI_Register_datarep")));
#endif

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Register_datarep_c = PMPI_Register_datarep_c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Register_datarep_c MPI_Register_datarep_c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Register_datarep_c as PMPI_Register_datarep_c
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Register_datarep_c(const char *datarep,
                           MPI_Datarep_conversion_function_c * read_conversion_fn,
                           MPI_Datarep_conversion_function_c * write_conversion_fn,
                           MPI_Datarep_extent_function * dtype_file_extent_fn, void *extra_state)
    __attribute__ ((weak, alias("PMPI_Register_datarep_c")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
  MPI_Register_datarep - Register functions for user-defined data
                         representations

Input Parameters:
+ datarep - data representation name (string)
. read_conversion_fn - function invoked to convert from file representation to
                 native representation (function)
. write_conversion_fn - function invoked to convert from native representation to
                  file representation (function)
. dtype_file_extent_fn - function invoked to get the extent of a datatype as represented
                  in the file (function)
- extra_state - pointer to extra state that is passed to each of the
                three functions

 Notes:
 This function allows the user to provide routines to convert data from
 an external representation, used within a file, and the native representation,
 used within the CPU.  There is one predefined data representation,
 'external32'.  Please consult the MPI-2 standard for details on this
 function.

.N fortran

  @*/

int MPI_Register_datarep(ROMIO_CONST char *datarep,
                         MPI_Datarep_conversion_function * read_conversion_fn,
                         MPI_Datarep_conversion_function * write_conversion_fn,
                         MPI_Datarep_extent_function * dtype_file_extent_fn, void *extra_state)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_Register_datarep_impl(datarep, read_conversion_fn, write_conversion_fn,
                                            dtype_file_extent_fn, extra_state);
    if (error_code) {
        goto fn_fail;
    }

  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return error_code;
  fn_fail:
    error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
    goto fn_exit;
}

/* large count function */


/*@
  MPI_Register_datarep_c - Register functions for user-defined data
                         representations

Input Parameters:
+ datarep - data representation name (string)
. read_conversion_fn - function invoked to convert from file representation to
                 native representation (function)
. write_conversion_fn - function invoked to convert from native representation to
                  file representation (function)
. dtype_file_extent_fn - function invoked to get the extent of a datatype as represented
                  in the file (function)
- extra_state - pointer to extra state that is passed to each of the
                three functions

 Notes:
 This function allows the user to provide routines to convert data from
 an external representation, used within a file, and the native representation,
 used within the CPU.  There is one predefined data representation,
 'external32'.  Please consult the MPI-2 standard for details on this
 function.

.N fortran

  @*/

int MPI_Register_datarep_c(ROMIO_CONST char *datarep,
                           MPI_Datarep_conversion_function_c * read_conversion_fn,
                           MPI_Datarep_conversion_function_c * write_conversion_fn,
                           MPI_Datarep_extent_function * dtype_file_extent_fn, void *extra_state)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_Register_datarep_large_impl(datarep, read_conversion_fn, write_conversion_fn,
                                                  dtype_file_extent_fn, extra_state);
    if (error_code) {
        goto fn_fail;
    }

  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return error_code;
  fn_fail:
    error_code = MPIO_Err_return_file(MPI_FILE_NULL, error_code);
    goto fn_exit;
}
