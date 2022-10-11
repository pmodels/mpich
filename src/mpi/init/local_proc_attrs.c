/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ERROR_CHECKING
      category    : ERROR_HANDLING
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, perform checks for errors, typically to verify valid inputs
        to MPI routines.  Only effective when MPICH is configured with
        --enable-error-checking=runtime .

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPII_init_local_proc_attrs(int *p_thread_required)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPICH_IS_THREADED
    /* If the user requested for asynchronous progress, request for
     * THREAD_MULTIPLE. */
    if (MPIR_CVAR_ASYNC_PROGRESS)
        *p_thread_required = MPI_THREAD_MULTIPLE;

    /* We need this inorder to implement IS_THREAD_MAIN */
#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
    MPID_Thread_self(&MPIR_ThreadInfo.main_thread);
#endif
#endif /* MPICH_IS_THREADED */

#ifdef HAVE_ERROR_CHECKING
    /* Because the PARAM system has not been initialized, temporarily
     * unconditionally enable error checks.  Once the PARAM system is
     * initialized, this may be reset */
    MPIR_Process.do_error_checks = 1;
#if (HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_RUNTIME)
    MPIR_Process.do_error_checks = MPIR_CVAR_ERROR_CHECKING;
#endif
#else
    MPIR_Process.do_error_checks = 0;
#endif

    /* Initialize necessary subsystems and setup the predefined attribute
     * values.  Subsystems may change these values. */
    MPIR_Process.attrs.appnum = -1;
    MPIR_Process.attrs.host = MPI_PROC_NULL;
    MPIR_Process.attrs.io = MPI_PROC_NULL;
    MPIR_Process.attrs.lastusedcode = MPI_ERR_LASTCODE;
    MPIR_Process.attrs.universe = MPIR_UNIVERSE_SIZE_NOT_SET;
    MPIR_Process.attrs.wtime_is_global = 0;

    /* Set the functions used to duplicate attributes.  These are
     * when the first corresponding keyval is created */
    MPIR_Process.attr_dup = 0;
    MPIR_Process.attr_free = 0;

    /* This allows the device to select an alternative function for
     * dimsCreate */
    MPIR_Process.dimsCreate = 0;

    /* Init communicator hints */
    MPIR_Comm_hint_init();

    MPIR_Process.comm_parent = NULL;

    /* Set the number of tag bits. The device may override this value. */
    MPIR_Process.tag_bits = MPIR_TAG_BITS_DEFAULT;

    return mpi_errno;
}

int MPII_init_tag_ub(void)
{
    /* Set tag_ub as function of tag_bits set by the device */
    MPIR_Process.attrs.tag_ub = MPIR_TAG_USABLE_BITS;

    /* TODO: turn assertions into error code */
    /* Assert: tag_ub should be a power of 2 minus 1 */
    MPIR_Assert(((unsigned) MPIR_Process.
                 attrs.tag_ub & ((unsigned) MPIR_Process.attrs.tag_ub + 1)) == 0);

    /* Assert: tag_ub is at least the minimum asked for in the MPI spec */
    MPIR_Assert(MPIR_Process.attrs.tag_ub >= 32767);

    return MPI_SUCCESS;
}

int MPII_init_builtin_infos(void)
{
    /* Init MPI_INFO_ENV object */
    MPIR_Info *info_ptr;
    MPIR_Info_get_ptr(MPI_INFO_ENV, info_ptr);
    info_ptr->handle = MPI_INFO_ENV;
    MPIR_Object_set_ref(info_ptr, 1);
    /* Add data to MPI_INFO_ENV. */
    MPIR_Info_setup_env(info_ptr);

    return MPI_SUCCESS;
}

int MPII_finalize_builtin_infos(void)
{
    MPIR_Info *info_ptr = NULL;
    MPIR_Info_get_ptr(MPI_INFO_ENV, info_ptr);
    return MPIR_Info_free_impl(info_ptr);
}
