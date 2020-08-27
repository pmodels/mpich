
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

typedef void (*generic_mpi_func) (void);

int MPII_qmpi_setup(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Allocate space for as many tools as we say we support */
    MPIR_QMPI_pointers = MPL_calloc(1, sizeof(generic_mpi_func *) * MPI_LAST_FUNC *
                                    (MPIR_QMPI_DEFAULT_NUM_TOOLS + 1), MPL_MEM_OTHER);
    if (!MPIR_QMPI_pointers) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_OTHER, "**nomem", "**nomem");
        goto fn_exit;
    }

    MPIR_QMPI_contexts = MPL_calloc(1, sizeof(void *) * (MPIR_QMPI_DEFAULT_NUM_TOOLS + 1),
                                    MPL_MEM_OTHER);
    if (!MPIR_QMPI_contexts) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_OTHER, "**nomem", "**nomem");
        goto fn_exit;
    }

    /* Set the pointers for the MPICH functions in the first position. */
    MPIR_QMPI_pointers[MPI_SEND_T] = (void (*)(void)) &QMPI_Send;
    MPIR_QMPI_pointers[MPI_RECV_T] = (void (*)(void)) &QMPI_Recv;

    MPIR_QMPI_contexts[0] = NULL;

    MPIR_QMPI_is_initialized = 1;

  fn_exit:
    return mpi_errno;
}

/*@
   QMPIX_Register - Register a QMPI tool.

Output Parameters:
. funcs - A struct of functions that includes every function for every attached QMPI tool, in
addition to the MPI library itself.

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int QMPIX_Register_tool(void *tool_context, int *tool_id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_QMPIX_REGISTER);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_QMPIX_REGISTER);

    /* ... body of routine ...  */

    /* TODO - Add a CVAR to change the default number of tools and check that we didn't run out */

    /* If the QMPI globals have not yet been set up, do so now. */
    if (!MPIR_QMPI_is_initialized) {
        MPII_qmpi_setup();
    }

    MPIR_QMPI_num_tools++;

    /* Store the tool's context information */
    MPIR_QMPI_contexts[MPIR_QMPI_num_tools] = tool_context;

    /* Return all the appropriate pointers back to the tool */
    *tool_id = MPIR_QMPI_num_tools;

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_QMPIX_REGISTER);

    return mpi_errno;

    /* ... end of body of routine ... */
}

int QMPIX_Register_function(int calling_tool_id, int function_enum, void (*function_ptr) (void))
{
    int mpi_errno = MPI_SUCCESS;

    if (function_enum < 0 && function_enum >= MPI_LAST_FUNC) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_ARG, "**arg", "**arg");
        goto fn_exit;
    }

    MPIR_QMPI_pointers[calling_tool_id * MPI_LAST_FUNC + function_enum] = function_ptr;

  fn_exit:
    return mpi_errno;
}

int QMPIX_Get_function(int calling_tool_id, int function_enum, void (**function_ptr) (void),
                       void **next_tool_context)
{
    int mpi_errno = MPI_SUCCESS;

    *function_ptr = NULL;

    if (function_enum < 0 && function_enum >= MPI_LAST_FUNC) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_ARG, "**arg", "**arg");
        goto fn_fail;
    }

    for (int i = calling_tool_id; i >= 0; i--) {
        if (MPIR_QMPI_pointers[i * MPI_LAST_FUNC + function_enum] != NULL) {
            *function_ptr = MPIR_QMPI_pointers[i * MPI_LAST_FUNC + function_enum];
            *next_tool_context = MPIR_QMPI_contexts[i];
        }
    }

    if (unlikely(*next_tool_context == NULL)) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_ARG, "**arg", "**arg");
    }

  fn_fail:
    return mpi_errno;
}
