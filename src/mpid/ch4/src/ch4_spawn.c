/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_COMM_CONNECT_TIMEOUT
      category    : CH4
      type        : int
      default     : 180
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP_EQ
      description : >-
        The default time out period in seconds for a connection attempt to the
        server communicator where the named port exists but no pending accept.
        User can change the value for a specified connection through its info
        argument.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPID_Comm_spawn_multiple(int count, char *commands[], char **argvs[], const int maxprocs[],
                             MPIR_Info * info_ptrs[], int root, MPIR_Comm * comm_ptr,
                             MPIR_Comm ** intercomm, int errcodes[])
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    char port_name[MPI_MAX_PORT_NAME];
    memset(port_name, 0, sizeof(port_name));

    int total_num_processes = 0;
    int spawn_error = 0;
    int *pmi_errcodes = NULL;

    if (comm_ptr->rank == root) {
        for (int i = 0; i < count; i++) {
            total_num_processes += maxprocs[i];
        }

        pmi_errcodes = (int *) MPL_calloc(total_num_processes, sizeof(int), MPL_MEM_OTHER);
        MPIR_Assert(pmi_errcodes);

        /* NOTE: we can't do ERR JUMP here, or the later BCAST won't work */

        mpi_errno = MPID_Open_port(NULL, port_name);
        if (mpi_errno == MPI_SUCCESS) {
            struct MPIR_PMI_KEYVAL preput_keyval_vector;
            preput_keyval_vector.key = MPIDI_PARENT_PORT_KVSKEY;
            preput_keyval_vector.val = port_name;

            MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_PMI_MUTEX);
            mpi_errno = MPIR_pmi_spawn_multiple(count, commands, argvs, maxprocs, info_ptrs,
                                                1, &preput_keyval_vector, pmi_errcodes);
            MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_PMI_MUTEX);
            if (mpi_errno != MPI_SUCCESS) {
                spawn_error = 1;
            }
        } else {
            spawn_error = 1;
        }
    }

    int bcast_ints[2];
    if (comm_ptr->rank == root) {
        bcast_ints[0] = total_num_processes;
        bcast_ints[1] = spawn_error;
    }
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    mpi_errno = MPIR_Bcast(bcast_ints, 2, MPI_INT, root, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);
    if (comm_ptr->rank != root) {
        total_num_processes = bcast_ints[0];
        spawn_error = bcast_ints[1];
        pmi_errcodes = (int *) MPL_calloc(total_num_processes, sizeof(int), MPL_MEM_OTHER);
        MPIR_Assert(pmi_errcodes);
    }

    MPIR_ERR_CHKANDJUMP(spawn_error, mpi_errno, MPI_ERR_OTHER, "**spawn");

    int should_accept = 1;
    if (errcodes != MPI_ERRCODES_IGNORE) {
        mpi_errno =
            MPIR_Bcast(pmi_errcodes, total_num_processes, MPI_INT, root, comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        for (int i = 0; i < total_num_processes; i++) {
            errcodes[i] = pmi_errcodes[i];
            should_accept = should_accept && errcodes[i];
        }
        should_accept = !should_accept;
    }
    if (should_accept) {
        mpi_errno = MPID_Comm_accept(port_name, NULL, root, comm_ptr, intercomm);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (errcodes[0] != 0) {
        /* FIXME: An empty comm? How does it supposed to work? */
        mpi_errno = MPIR_Comm_create(intercomm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (comm_ptr->rank == root) {
        mpi_errno = MPID_Close_port(port_name);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPL_free(pmi_errcodes);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_connect(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                      MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int timeout = MPIR_CVAR_CH4_COMM_CONNECT_TIMEOUT;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_CONNECT);

    if (info != NULL) {
        int info_flag = 0;
        char info_value[MPI_MAX_INFO_VAL + 1];
        MPIR_Info_get_impl(info, "timeout", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            timeout = atoi(info_value);
        }
    }
    mpi_errno = MPIDI_NM_mpi_comm_connect(port_name, info, root, timeout, comm, newcomm_ptr);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_CONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_disconnect(MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_DISCONNECT);
    mpi_errno = MPIDI_NM_mpi_comm_disconnect(comm_ptr);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Open_port(MPIR_Info * info_ptr, char *port_name)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_OPEN_PORT);
    mpi_errno = MPIDI_NM_mpi_open_port(info_ptr, port_name);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_OPEN_PORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Close_port(const char *port_name)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CLOSE_PORT);
    mpi_errno = MPIDI_NM_mpi_close_port(port_name);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                     MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_ACCEPT);
    mpi_errno = MPIDI_NM_mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
