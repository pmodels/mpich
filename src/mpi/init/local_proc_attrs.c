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

    /* "Allocate" from the reserved space for builtin communicators and
     * (partially) initialize predefined communicators.  comm_parent is
     * initially NULL and will be allocated by the device if the process group
     * was started using one of the MPI_Comm_spawn functions. */
    MPIR_Process.comm_world = MPIR_Comm_builtin + 0;
    MPII_Comm_init(MPIR_Process.comm_world);
    MPIR_Process.comm_world->handle = MPI_COMM_WORLD;
    MPIR_Process.comm_world->context_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_world->recvcontext_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_world->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    /* This initialization of the comm name could be done only when
     * comm_get_name is called */
    MPL_strncpy(MPIR_Process.comm_world->name, "MPI_COMM_WORLD", MPI_MAX_OBJECT_NAME);

    MPIR_Process.comm_self = MPIR_Comm_builtin + 1;
    MPII_Comm_init(MPIR_Process.comm_self);
    MPIR_Process.comm_self->handle = MPI_COMM_SELF;
    MPIR_Process.comm_self->context_id = 1 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_self->recvcontext_id = 1 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.comm_self->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    MPL_strncpy(MPIR_Process.comm_self->name, "MPI_COMM_SELF", MPI_MAX_OBJECT_NAME);

#ifdef MPID_NEEDS_ICOMM_WORLD
    MPIR_Process.icomm_world = MPIR_Comm_builtin + 2;
    MPII_Comm_init(MPIR_Process.icomm_world);
    MPIR_Process.icomm_world->handle = MPIR_ICOMM_WORLD;
    MPIR_Process.icomm_world->context_id = 2 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.icomm_world->recvcontext_id = 2 << MPIR_CONTEXT_PREFIX_SHIFT;
    MPIR_Process.icomm_world->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    MPL_strncpy(MPIR_Process.icomm_world->name, "MPI_ICOMM_WORLD", MPI_MAX_OBJECT_NAME);

    /* Note that these communicators are not ready for use - MPID_Init
     * will setup self and world, and icomm_world if it desires it. */
#endif

    MPIR_Process.comm_parent = NULL;

    /* Setup the initial communicator list in case we have
     * enabled the debugger message-queue interface */
    MPII_COMML_REMEMBER(MPIR_Process.comm_world);
    MPII_COMML_REMEMBER(MPIR_Process.comm_self);

    /* create MPI_INFO_NULL object */
    MPIR_Info *info_ptr;
    info_ptr = MPIR_Info_builtin + 1;
    info_ptr->handle = MPI_INFO_ENV;
    MPIR_Object_set_ref(info_ptr, 1);
    /* Add data to MPI_INFO_ENV. */
    MPIR_Info_setup_env(info_ptr);
    info_ptr->next = NULL;
    info_ptr->key = NULL;
    info_ptr->value = NULL;

    /* Set the number of tag bits. The device may override this value. */
    MPIR_Process.tag_bits = MPIR_TAG_BITS_DEFAULT;

    /* Init communicator hints */
    MPIR_Comm_hint_init();

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

int MPII_finalize_local_proc_attrs(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Remove the attributes, executing the attribute delete routine.
     * Do this only if the attribute functions are defined. */
    /* The standard (MPI-2, section 4.8) says that the attributes on
     * MPI_COMM_SELF are deleted before almost anything else happens */
    /* Note that the attributes need to be removed from the communicators
     * so that they aren't freed twice. (The communicators are released
     * in MPID_Finalize) */
    if (MPIR_Process.attr_free && MPIR_Process.comm_self->attributes) {
        mpi_errno = MPIR_Process.attr_free(MPI_COMM_SELF, &MPIR_Process.comm_self->attributes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Process.comm_self->attributes = 0;
    }
    if (MPIR_Process.attr_free && MPIR_Process.comm_world->attributes) {
        mpi_errno = MPIR_Process.attr_free(MPI_COMM_WORLD, &MPIR_Process.comm_world->attributes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Process.comm_world->attributes = 0;
    }

    /*
     * Now that we're finalizing, we need to take control of the error handlers
     * At this point, we will release any user-defined error handlers on
     * comm self and comm world
     */
    if (MPIR_Process.comm_world->errhandler &&
        !(HANDLE_IS_BUILTIN(MPIR_Process.comm_world->errhandler->handle))) {
        int in_use;
        MPIR_Errhandler_release_ref(MPIR_Process.comm_world->errhandler, &in_use);
        if (!in_use) {
            MPIR_Handle_obj_free(&MPIR_Errhandler_mem, MPIR_Process.comm_world->errhandler);
        }
        /* always set to NULL to avoid a double-release later in finalize */
        MPIR_Process.comm_world->errhandler = NULL;
    }
    if (MPIR_Process.comm_self->errhandler &&
        !(HANDLE_IS_BUILTIN(MPIR_Process.comm_self->errhandler->handle))) {
        int in_use;
        MPIR_Errhandler_release_ref(MPIR_Process.comm_self->errhandler, &in_use);
        if (!in_use) {
            MPIR_Handle_obj_free(&MPIR_Errhandler_mem, MPIR_Process.comm_self->errhandler);
        }
        /* always set to NULL to avoid a double-release later in finalize */
        MPIR_Process.comm_self->errhandler = NULL;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
