/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#define COMM_WORLD_CTXID (0 << MPIR_CONTEXT_PREFIX_SHIFT)
#define COMM_SELF_CTXID  (1 << MPIR_CONTEXT_PREFIX_SHIFT)
#ifdef MPID_NEEDS_ICOMM_WORLD
#define ICOMM_WORLD_CTXID (2 << MPIR_CONTEXT_PREFIX_SHIFT)
#endif

int MPIR_init_comm_world(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Process.comm_world == NULL);

    MPIR_Process.comm_world = MPIR_Comm_builtin + 0;
    MPII_Comm_init(MPIR_Process.comm_world);

    MPIR_Process.comm_world->rank = MPIR_Process.rank;
    MPIR_Process.comm_world->handle = MPI_COMM_WORLD;
    MPIR_Process.comm_world->context_id = COMM_WORLD_CTXID;
    MPIR_Process.comm_world->recvcontext_id = MPIR_Process.comm_world->context_id;
    MPIR_Process.comm_world->comm_kind = MPIR_COMM_KIND__INTRACOMM;

    MPIR_Process.comm_world->rank = MPIR_Process.rank;
    MPIR_Process.comm_world->remote_size = MPIR_Process.size;
    MPIR_Process.comm_world->local_size = MPIR_Process.size;

    mpi_errno = MPIR_Comm_commit(MPIR_Process.comm_world);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_strncpy(MPIR_Process.comm_world->name, "MPI_COMM_WORLD", MPI_MAX_OBJECT_NAME);
    MPII_COMML_REMEMBER(MPIR_Process.comm_world);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_init_comm_self(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Process.comm_self == NULL);

    MPIR_Process.comm_self = MPIR_Comm_builtin + 1;
    MPII_Comm_init(MPIR_Process.comm_self);
    MPIR_Process.comm_self->handle = MPI_COMM_SELF;
    MPIR_Process.comm_self->context_id = COMM_SELF_CTXID;
    MPIR_Process.comm_self->recvcontext_id = MPIR_Process.comm_self->context_id;
    MPIR_Process.comm_self->comm_kind = MPIR_COMM_KIND__INTRACOMM;

    MPIR_Process.comm_self->rank = 0;
    MPIR_Process.comm_self->remote_size = 1;
    MPIR_Process.comm_self->local_size = 1;

    mpi_errno = MPIR_Comm_commit(MPIR_Process.comm_self);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_strncpy(MPIR_Process.comm_self->name, "MPI_COMM_SELF", MPI_MAX_OBJECT_NAME);
    MPII_COMML_REMEMBER(MPIR_Process.comm_self);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifdef MPID_NEEDS_ICOMM_WORLD
int MPIR_init_icomm_world(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Process.icomm_world == NULL);

    MPIR_Process.icomm_world = MPIR_Comm_builtin + 2;
    MPII_Comm_init(MPIR_Process.icomm_world);

    MPIR_Process.icomm_world->rank = MPIR_Process.rank;
    MPIR_Process.icomm_world->handle = MPIR_ICOMM_WORLD;
    MPIR_Process.icomm_world->context_id = ICOMM_WORLD_CTXID;
    MPIR_Process.icomm_world->recvcontext_id = MPIR_Process.icomm_world->context_id;
    MPIR_Process.icomm_world->comm_kind = MPIR_COMM_KIND__INTRACOMM;

    MPIR_Process.icomm_world->rank = MPIR_Process.rank;
    MPIR_Process.icomm_world->remote_size = MPIR_Process.size;
    MPIR_Process.icomm_world->local_size = MPIR_Process.size;

    mpi_errno = MPIR_Comm_commit(MPIR_Process.icomm_world);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_strncpy(MPIR_Process.icomm_world->name, "MPI_ICOMM_WORLD", MPI_MAX_OBJECT_NAME);
    MPII_COMML_REMEMBER(MPIR_Process.icomm_world);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif

static int finalize_builtin_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    /* Note that the attributes need to be removed from the communicators
     * so that they aren't freed twice.
     */
    if (MPIR_Process.attr_free && comm->attributes) {
        mpi_errno = MPIR_Process.attr_free(comm->handle, &comm->attributes);
        MPIR_ERR_CHECK(mpi_errno);
        comm->attributes = 0;
    }

    if (comm->errhandler && !(HANDLE_IS_BUILTIN(comm->errhandler->handle))) {
        int in_use;
        MPIR_Errhandler_release_ref(comm->errhandler, &in_use);
        if (!in_use) {
            MPIR_Handle_obj_free(&MPIR_Errhandler_mem, comm->errhandler);
        }
        /* always set to NULL to avoid a double-release later in finalize */
        comm->errhandler = NULL;
    }

    MPIR_Comm_free_inactive_requests(comm);

    int ref_count = MPIR_Object_get_ref(comm);
    if (ref_count != 1) {
        MPL_internal_error_printf
            ("WARNING: Builtin communicator %x has pending %d references, polling progress until all references clears.\n",
             comm->handle, ref_count - 1);
        while (MPIR_Object_get_ref(comm) > 1) {
            MPID_Stream_progress(NULL);
        }
    }
    mpi_errno = MPIR_Comm_release(comm);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_finalize_builtin_comms(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* The standard (MPI-2, section 4.8) says that the attributes on
     * MPI_COMM_SELF are deleted before almost anything else happens */
    if (MPIR_Process.comm_self) {
        mpi_errno = finalize_builtin_comm(MPIR_Process.comm_self);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Process.comm_self = NULL;
    } else {
        MPIR_Free_contextid(COMM_SELF_CTXID);
    }

    if (MPIR_Process.comm_world) {
        mpi_errno = finalize_builtin_comm(MPIR_Process.comm_world);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Process.comm_world = NULL;
    } else {
        MPIR_Free_contextid(COMM_WORLD_CTXID);
    }

    if (MPIR_Process.comm_parent) {
        mpi_errno = finalize_builtin_comm(MPIR_Process.comm_parent);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Process.comm_parent = NULL;
    }
#ifdef MPID_NEEDS_ICOMM_WORLD
    if (MPIR_Process.icomm_world) {
        mpi_errno = finalize_builtin_comm(MPIR_Process.icomm_world);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Process.icomm_world = NULL;
    } else {
        MPIR_Free_contextid(ICOMM_WORLD_CTXID);
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
