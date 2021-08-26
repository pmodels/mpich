
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_QMPI

#include "qmpi_register.h"
#include <execinfo.h>
#define BACKTRACE_SIZE 5

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : TOOLS
      description : cvars that control tools connected to MPICH

cvars:
    - name        : MPIR_CVAR_QMPI_TOOL_LIST
      alias       : QMPI_TOOL_LIST
      category    : TOOLS
      type        : string
      default     : NULL
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Set the number and order of QMPI tools to be loaded by the MPI library when it is
        initialized.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

typedef void (*generic_mpi_func) (void);

void (**MPIR_QMPI_pointers) (void);
void **MPIR_QMPI_storage;
void (**MPIR_QMPI_tool_init_callbacks) (int);
int MPIR_QMPI_num_tools = 0;
char **MPIR_QMPI_tool_names;
void (**MPIR_QMPI_first_fn_ptrs) (void);
int *MPIR_QMPI_first_tool_ids;

/* Cache the first function and tool ID in the chain of QMPI tools to optimize the lookup in all of
 * the internal MPI_* functions. */
int MPII_qmpi_stash_first_tools(void)
{
    if (MPIR_QMPI_num_tools == 0) {
        return MPI_SUCCESS;
    }

    for (int i = 0; i < MPI_LAST_FUNC_T; i++) {
        MPIR_QMPI_first_fn_ptrs[i] = NULL;

        for (int j = 1; j <= MPIR_QMPI_num_tools; j++) {
            if (MPIR_QMPI_pointers[j * MPI_LAST_FUNC_T + i] != NULL) {
                MPIR_QMPI_first_fn_ptrs[i] = MPIR_QMPI_pointers[j * MPI_LAST_FUNC_T + i];
                MPIR_QMPI_first_tool_ids[i] = j;
                break;
            }
        }

        if (MPIR_QMPI_first_fn_ptrs[i] == NULL) {
            MPIR_QMPI_first_fn_ptrs[i] = MPIR_QMPI_pointers[i];
            MPIR_QMPI_first_tool_ids[i] = 0;
        }
    }
    return MPI_SUCCESS;
}

int MPII_qmpi_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    static bool qmpi_initialized = false;

    if (!qmpi_initialized) {
        qmpi_initialized = true;

        /* Call this function in case it is needed */
        mpi_errno = MPII_qmpi_pre_init();
        MPIR_ERR_CHECK(mpi_errno);

        /* Initialize the tools that have been registered */
        for (int i = 1; i < MPIR_QMPI_num_tools + 1; i++) {
            MPIR_QMPI_tool_init_callbacks[i] (i);
        }

        mpi_errno = MPII_qmpi_stash_first_tools();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_qmpi_pre_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    size_t len = 0;

    static bool qmpi_pre_initialized = false;

    if (!qmpi_pre_initialized) {
        qmpi_pre_initialized = true;

        mpi_errno = MPIR_T_env_init();
        MPIR_ERR_CHECK(mpi_errno);

        /* Parse environment variable to get the number and list of tools */
        if (MPIR_CVAR_QMPI_TOOL_LIST == NULL) {
            MPIR_QMPI_num_tools = 0;
            goto fn_exit;
        } else {
            MPIR_QMPI_num_tools = 1;
            len = strlen(MPIR_CVAR_QMPI_TOOL_LIST);
        }

        for (int i = 0; i < len; i++) {
            if (MPIR_CVAR_QMPI_TOOL_LIST[i] == ':') {
                MPIR_QMPI_num_tools++;
            }
        }
        MPIR_QMPI_tool_names = MPL_calloc(MPIR_QMPI_num_tools + 1, sizeof(char *), MPL_MEM_OTHER);
        if (!MPIR_QMPI_tool_names) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_num_tools");
            goto fn_exit;
        }
        for (int i = 0; i < MPIR_QMPI_num_tools + 1; i++) {
            MPIR_QMPI_tool_names[i] =
                MPL_calloc(QMPI_MAX_TOOL_NAME_LENGTH, sizeof(char), MPL_MEM_OTHER);
            if (!MPIR_QMPI_tool_names[i]) {
                MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_num_tools");
                goto fn_exit;
            }
        }

        char *save_ptr, *val;
        char *tmp_str = MPL_strdup(MPIR_CVAR_QMPI_TOOL_LIST);
        val = strtok_r(tmp_str, ":", &save_ptr);
        int tool_num = 1;       /* Counter for the ID of each tool. MPI itself is "tool 0". */
        while (val != NULL) {
            strncpy(MPIR_QMPI_tool_names[tool_num], val, QMPI_MAX_TOOL_NAME_LENGTH);
            /* Make sure the string is null terminated */
            MPIR_QMPI_tool_names[tool_num][QMPI_MAX_TOOL_NAME_LENGTH - 1] = '\0';
            tool_num++;
            val = strtok_r(NULL, ":", &save_ptr);
        }
        MPL_free(tmp_str);
        MPIR_QMPI_num_tools = tool_num - 1;

        /* Get the number of MPI functions that we have to track. */
        int num_funcs = MPI_LAST_FUNC_T;

        /* For each of the allocations below, we actually allocate one extra entry, but it
         * simplifies things to have a single numbering system that can just use the tool IDs as the
         * index into the arrays. */

        /* Allocate space for as many tools as we say we support */
        MPIR_QMPI_pointers = MPL_calloc(1, sizeof(generic_mpi_func *) * num_funcs * tool_num,
                                        MPL_MEM_OTHER);
        if (!MPIR_QMPI_pointers) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_pointers");
            goto fn_exit;
        }

        /* Allocate space to stash the first callback function in the chain */
        MPIR_QMPI_first_fn_ptrs = MPL_calloc(1, sizeof(generic_mpi_func *) * num_funcs,
                                             MPL_MEM_OTHER);
        if (!MPIR_QMPI_first_fn_ptrs) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_pointers");
            goto fn_exit;
        }

        /* Allocate space to stash the first tool ID in the chain */
        MPIR_QMPI_first_tool_ids = MPL_calloc(1, sizeof(int) * num_funcs, MPL_MEM_OTHER);
        if (!MPIR_QMPI_first_tool_ids) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_pointers");
            goto fn_exit;
        }

        MPIR_QMPI_storage = MPL_calloc(1, sizeof(void *) * tool_num, MPL_MEM_OTHER);
        if (!MPIR_QMPI_storage) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_storage");
            goto fn_exit;
        }

        MPIR_QMPI_tool_init_callbacks = MPL_calloc(1, sizeof(generic_mpi_func *) * num_funcs *
                                                   tool_num, MPL_MEM_OTHER);
        if (!MPIR_QMPI_tool_init_callbacks) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_pointers");
            goto fn_exit;
        }

        /* Call generated function to register the internal function pointers. */
        MPII_qmpi_register_internal_functions();

        MPIR_QMPI_storage[0] = NULL;

        mpi_errno = MPII_qmpi_stash_first_tools();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPII_qmpi_teardown(void)
{
    int counter = 0;

    MPL_free(MPIR_QMPI_pointers);
    MPL_free(MPIR_QMPI_storage);
    if (MPIR_CVAR_QMPI_TOOL_LIST != NULL) {
        /* We allocate one extra tool name so we don't have to shift to avoid the MPICH functions */
        MPL_free(MPIR_QMPI_tool_names[counter++]);
        MPL_free(MPIR_QMPI_tool_names[counter++]);
        size_t len = strlen(MPIR_CVAR_QMPI_TOOL_LIST);
        for (int i = 0; i <= len; i++) {
            if (MPIR_CVAR_QMPI_TOOL_LIST[i] == ':') {
                MPL_free(MPIR_QMPI_tool_names[counter++]);
            }
        }
    }
    MPL_free(MPIR_QMPI_first_fn_ptrs);
    MPL_free(MPIR_QMPI_first_tool_ids);
    MPL_free(MPIR_QMPI_tool_names);
    MPL_free(MPIR_QMPI_tool_init_callbacks);
}

/*@
   QMPI_Register - Register a QMPI tool.

Output Parameters:
. funcs - A struct of functions that includes every function for every attached QMPI tool, in
addition to the MPI library itself.

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int QMPI_Register_tool_name(const char *tool_name, void (*init_function_ptr) (int tool_id))
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* If the QMPI globals have not yet been set up, do so now. This does not need to be protected
     * by a mutex because it will be called before the application has even reached its main
     * function. */
    MPII_qmpi_pre_init();

    if (MPIR_QMPI_num_tools) {
        /* Copy the tool information into internal arrays */
        bool found = false;
        for (int i = 1; i <= MPIR_QMPI_num_tools; i++) {
            if (strcmp(MPIR_QMPI_tool_names[i], tool_name) == 0 &&
                MPIR_QMPI_tool_init_callbacks[i] == NULL) {
                MPIR_QMPI_tool_init_callbacks[i] = init_function_ptr;
                found = true;
            }
        }
        if (!found) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**qmpi_invalid_name", "**qmpi_invalid_name %s",
                                 tool_name);
            goto fn_exit;
        }
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int QMPI_Register_tool_storage(int tool_id, void *tool_storage)
{
    int mpi_errno = MPI_SUCCESS;

    /* Store the tool's context information */
    MPIR_QMPI_storage[tool_id] = tool_storage;

    return mpi_errno;
}

int QMPI_Register_function(int tool_id, enum QMPI_Functions_enum function_enum,
                           void (*function_ptr) (void))
{
    int mpi_errno = MPI_SUCCESS;

    if (function_enum < 0 && function_enum >= MPI_LAST_FUNC_T) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_ARG, "**arg", "**arg %s", "invalid enum value");
        goto fn_exit;
    }

    MPIR_QMPI_pointers[tool_id * MPI_LAST_FUNC_T + function_enum] = function_ptr;

  fn_exit:
    return mpi_errno;
}

int QMPI_Get_function(int calling_tool_id, enum QMPI_Functions_enum function_enum,
                      void (**function_ptr) (void), int *next_tool_id)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = calling_tool_id + 1; i <= MPIR_QMPI_num_tools; i++) {
        if (MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum] != NULL) {
            *function_ptr = MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum];
            *next_tool_id = i;
            return mpi_errno;
        }
    }

    *function_ptr = MPIR_QMPI_pointers[function_enum];
    *next_tool_id = 0;

    return mpi_errno;
}

int QMPI_Get_tool_storage(QMPI_Context context, int tool_id, void **storage)
{
    int mpi_errno = MPI_SUCCESS;

    *storage = MPIR_QMPI_storage[tool_id];

    return mpi_errno;
}

int QMPI_Get_calling_address(QMPI_Context context, void **address)
{
    int mpi_errno = MPI_SUCCESS;

    void *array[BACKTRACE_SIZE];
    char **traces;
    int size, i;
    int trace_entry = 1;        /* pick entry at 1 by default */
    char *target = NULL;

    /* Capture the calling function address */
    size = backtrace(array, BACKTRACE_SIZE);
    traces = backtrace_symbols(array, size);
    if (traces != NULL) {
        for (i = 0; i < size; i++) {
            /* Skip PMPI_* calls */
            char search[] = "PMPI_";
            char *found = strstr(traces[i], search);
            if (found != NULL) {
                trace_entry += i;
                break;
            }
        }
        /* Extract the address from the backtrace */
        if (size > trace_entry) {
            const char *start_pattern = "[";
            const char *end_pattern = "]";

            char *start, *end;
            start = strstr(traces[trace_entry], start_pattern);
            if (NULL != start) {
                start += strlen(start_pattern);
                end = strstr(start, end_pattern);
                if (NULL != end) {
                    target = (char *) MPL_malloc(end - start + 1, MPL_MEM_OTHER);
                    memcpy(target, start, end - start);
                    target[end - start] = '\0';
                }
            }
        }
    }
    MPL_free(traces);
    /* Store the calling address */
    *address = MPL_calloc(0, strlen(target) + 1, MPL_MEM_OTHER);
    memcpy(*address, target, strlen(target) + 1);
    MPL_free(target);

    return mpi_errno;
}

#else

int QMPI_Register_tool_name(const char *tool_name, void (*init_function_ptr) (int tool_id))
{
    return MPI_ERR_UNSUPPORTED_OPERATION;
}

int QMPI_Register_tool_storage(int tool_id, void *tool_storage)
{
    return MPI_ERR_UNSUPPORTED_OPERATION;
}

int QMPI_Register_function(int tool_id, enum QMPI_Functions_enum function_enum,
                           void (*function_ptr) (void))
{
    return MPI_ERR_UNSUPPORTED_OPERATION;
}

int QMPI_Get_function(int calling_tool_id, enum QMPI_Functions_enum function_enum,
                      void (**function_ptr) (void), int *next_tool_id)
{
    return MPI_ERR_UNSUPPORTED_OPERATION;
}

int QMPI_Get_tool_storage(QMPI_Context context, int tool_id, void **storage)
{
    return MPI_ERR_UNSUPPORTED_OPERATION;
}

int QMPI_Get_calling_address(QMPI_Context context, void **address)
{
    return MPI_ERR_UNSUPPORTED_OPERATION;
}

#endif /* ENABLE_QMPI */
