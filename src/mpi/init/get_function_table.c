
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*@
   QMPIX_Get_function_table - Gets the QMPI function table to allowt the user to determine the next
                              function or functions to call.

Output Parameters:
. funcs - A struct of functions that includes every function for every attached QMPI tool, in
addition to the MPI library itself.

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
QMPI_Function_pointers_t **QMPIX_Get_function_table(void)
{
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_QMPIX_GET_FUNCTION_TABLE);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_QMPIX_GET_FUNCTION_TABLE);

    /* ... body of routine ...  */

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_QMPIX_GET_FUNCTION_TABLE);
    return MPIR_QMPI_pointers;

    /* ... end of body of routine ... */
}
