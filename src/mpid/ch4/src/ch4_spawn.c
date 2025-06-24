/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

#define MPIDI_DYNPROC_NAME_MAX MPID_MAX_BC_SIZE

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

    MPIR_FUNC_ENTER;

    char port_name[MPID_MAX_PORT_NAME];
    memset(port_name, 0, sizeof(port_name));

    int total_num_processes = 0;
    int *pmi_errcodes = NULL;

    if (comm_ptr->rank == root) {
        for (int i = 0; i < count; i++) {
            total_num_processes += maxprocs[i];
        }

        pmi_errcodes = (int *) MPL_calloc(total_num_processes, sizeof(int), MPL_MEM_OTHER);
        MPIR_Assert(pmi_errcodes);

        /* NOTE: we can't do ERR JUMP here, or the later BCAST won't work */

        mpi_errno = MPID_Open_port(NULL, port_name, MPID_MAX_PORT_NAME);
        if (mpi_errno == MPI_SUCCESS) {
            struct MPIR_PMI_KEYVAL preput_keyval_vector;
            preput_keyval_vector.key = MPIDI_PARENT_PORT_KVSKEY;
            preput_keyval_vector.val = port_name;

            MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_PMI_MUTEX);
            mpi_errno = MPIR_pmi_spawn_multiple(count, commands, argvs, maxprocs, info_ptrs,
                                                1, &preput_keyval_vector, pmi_errcodes);
            MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_PMI_MUTEX);
        }
    }

    int bcast_ints[2];
    if (comm_ptr->rank == root) {
        bcast_ints[0] = total_num_processes;
        bcast_ints[1] = mpi_errno;
    }
    mpi_errno = MPIR_Bcast(bcast_ints, 2, MPIR_INT_INTERNAL, root, comm_ptr, MPIR_COLL_ATTR_SYNC);
    MPIR_ERR_CHECK(mpi_errno);
    if (comm_ptr->rank != root) {
        total_num_processes = bcast_ints[0];
        if (bcast_ints[1]) {
            /* root spawn encountered error */
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**spawn");
        }
        pmi_errcodes = (int *) MPL_calloc(total_num_processes, sizeof(int), MPL_MEM_OTHER);
        MPIR_Assert(pmi_errcodes);
    } else {
        /* restore the potential mpi_errno from MPID_Open_port and MPIR_pmi_spawn_multiple */
        mpi_errno = bcast_ints[1];
    }

    MPIR_ERR_CHECK(mpi_errno);

    int should_accept = 1;
    if (errcodes != MPI_ERRCODES_IGNORE) {
        mpi_errno = MPIR_Bcast(pmi_errcodes, total_num_processes, MPIR_INT_INTERNAL,
                               root, comm_ptr, MPIR_COLL_ATTR_SYNC);
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

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPID_Open_port, MPID_Close_port */

#define PORT_NAME_TAG_KEY "tag"
#define CONNENTR_TAG_KEY "connentry"

static void free_port_name_tag(int tag);
static int get_port_name_tag(int *port_name_tag);
static int get_tag_from_port(const char *port_name, int *port_name_tag);
static int get_conn_name_from_port(const char *port_name, char *connname, int maxlen, int *len);

#define PORT_MASK_BIT(j) (1u << ((8 * sizeof(int)) - j - 1))
static int port_name_tag_mask[MPIR_MAX_CONTEXT_MASK];

static void free_port_name_tag(int tag)
{
    int idx, rem_tag;

    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    idx = tag / (sizeof(int) * 8);
    rem_tag = tag - (idx * sizeof(int) * 8);

    port_name_tag_mask[idx] &= ~PORT_MASK_BIT(rem_tag);

    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_EXIT;
}

static int get_port_name_tag(int *port_name_tag)
{
    unsigned i, j;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    for (i = 0; i < MPIR_MAX_CONTEXT_MASK; i++)
        if (port_name_tag_mask[i] != ~0)
            break;

    if (i < MPIR_MAX_CONTEXT_MASK) {
        for (j = 0; j < (8 * sizeof(int)); j++) {
            if ((port_name_tag_mask[i] | PORT_MASK_BIT(j)) != port_name_tag_mask[i]) {
                port_name_tag_mask[i] |= PORT_MASK_BIT(j);
                *port_name_tag = ((i * 8 * sizeof(int)) + j);
                goto fn_exit;
            }
        }
    } else {
        goto fn_fail;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    *port_name_tag = -1;
    mpi_errno = MPI_ERR_OTHER;
    goto fn_exit;
}

static int get_tag_from_port(const char *port_name, int *port_name_tag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (strlen(port_name) == 0)
        goto fn_exit;

    int err;
    err = MPL_str_get_int_arg(port_name, PORT_NAME_TAG_KEY, port_name_tag);
    MPIR_ERR_CHKANDJUMP(err, mpi_errno, MPI_ERR_OTHER, "**argstr_no_port_name_tag");
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_conn_name_from_port(const char *port_name, char *connname, int maxlen, int *len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int outlen;
    int err;
    err = MPL_str_get_binary_arg(port_name, CONNENTR_TAG_KEY, connname, maxlen, &outlen);
    MPIR_ERR_CHKANDJUMP(err, mpi_errno, MPI_ERR_OTHER, "**argstr_no_port_name_tag");
    *len = outlen;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Open_port(MPIR_Info * info_ptr, char *port_name, int len)
{
    int mpi_errno;
    char *addrname = NULL;
    int *addrname_size = NULL;
    MPIR_FUNC_ENTER;

    int tag = -1;
    mpi_errno = get_port_name_tag(&tag);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));
    mpi_errno = MPIDI_NM_get_local_upids(MPIR_Process.comm_self, &addrname_size, &addrname);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    MPIR_ERR_CHECK(mpi_errno);

    int err;
    err = MPL_str_add_int_arg(&port_name, &len, PORT_NAME_TAG_KEY, tag);
    MPIR_ERR_CHKANDJUMP(err, mpi_errno, MPI_ERR_OTHER, "**argstr_port_name_tag");
    err = MPL_str_add_binary_arg(&port_name, &len, CONNENTR_TAG_KEY, addrname, addrname_size[0]);
    MPIR_ERR_CHKANDJUMP(err, mpi_errno, MPI_ERR_OTHER, "**argstr_port_name_tag");

  fn_exit:
    MPL_free(addrname);
    MPL_free(addrname_size);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Close_port(const char *port_name)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    int tag;
    mpi_errno = get_tag_from_port(port_name, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    free_port_name_tag(tag);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPID_Comm_accept, MPID_Comm_connect */

static int establish_peer_conn(char *remote_addrname, int remote_addrname_len, int tag,
                               int timeout, bool is_sender, MPIR_Lpid * remote_lpid_out)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Lpid remote_lpid = MPIR_LPID_INVALID;

    struct dynproc_conn_hdr {
        int addrname_len;
        char addrname[MPIDI_DYNPROC_NAME_MAX];
    } hdr;

    /* We enter the LOCK to ensure the dynamic exchange don't get interleaved.
     * NOTE: most other functions enter lock at NM-layer except the the functions defined
     *       in e.g. ofi_spawn.c and ucx_spawn.c. So only those functions are allowed
     *       inside the CS.
     */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));

    if (is_sender) {
        /* insert remote address */
        mpi_errno = MPIDIU_insert_dynamic_upid(&remote_lpid, remote_addrname, remote_addrname_len);
        MPIR_ERR_CHECK(mpi_errno);

        /* get my addrname and send it to remote */
        char *my_addrname;
        int *my_addrname_len;
        mpi_errno = MPIDI_NM_get_local_upids(MPIR_Process.comm_self,
                                             &my_addrname_len, &my_addrname);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(my_addrname_len[0] <= MPIDI_DYNPROC_NAME_MAX);
        memcpy(hdr.addrname, my_addrname, my_addrname_len[0]);
        hdr.addrname_len = my_addrname_len[0];

        /* send it to remote */
        int hdr_sz = sizeof(hdr) - MPIDI_DYNPROC_NAME_MAX + hdr.addrname_len;
        mpi_errno = MPIDI_NM_dynamic_send(remote_lpid, tag, &hdr, hdr_sz, timeout);
        MPL_free(my_addrname);
        MPL_free(my_addrname_len);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* recv remote address */
        mpi_errno = MPIDI_NM_dynamic_recv(tag, &hdr, sizeof(hdr), timeout);
        MPIR_ERR_CHECK(mpi_errno);

        /* insert remote address */
        mpi_errno = MPIDIU_insert_dynamic_upid(&remote_lpid, hdr.addrname, hdr.addrname_len);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    *remote_lpid_out = remote_lpid;
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynamic_intercomm_create(const char *port_name, MPIR_Info * info, int root,
                                    MPIR_Comm * comm_ptr, int timeout, bool is_sender,
                                    MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Lpid remote_lpid = MPIR_LPID_INVALID;
    MPIR_Comm *peer_comm = NULL;
    int tag;
    if (comm_ptr->rank == root) {
        /* NOTE: do not goto fn_fail on error, or it will leave children hanging */
        mpi_errno = get_tag_from_port(port_name, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        char remote_addrname[MPIDI_DYNPROC_NAME_MAX];
        char *addrname;
        int len;
        if (is_sender) {
            addrname = remote_addrname;
            mpi_errno = get_conn_name_from_port(port_name, remote_addrname, MPIDI_DYNPROC_NAME_MAX,
                                                &len);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            addrname = NULL;
            len = 0;
        }

        mpi_errno = establish_peer_conn(addrname, len, tag, timeout, is_sender, &remote_lpid);
        MPIR_ERR_CHECK(mpi_errno);

        /* create peer intercomm -
         * Since we will only use peer intercomm to call back MPID_Intercomm_exchange, which
         * just need to extract remote_lpid from the peer_comm, we can cheat a bit here - just
         * fill peer_comm->remote_group.
         */
        peer_comm = (MPIR_Comm *) MPIR_Handle_obj_alloc(&MPIR_Comm_mem);
        MPIR_ERR_CHKANDJUMP(!peer_comm, mpi_errno, MPI_ERR_OTHER, "**nomem");

        peer_comm->comm_kind = MPIR_COMM_KIND__INTERCOMM;
        peer_comm->remote_size = 1;
        peer_comm->local_size = 1;
        peer_comm->rank = 0;
        peer_comm->local_group = NULL;

        MPIR_Group_create_stride(1, 0, NULL, remote_lpid, 1, &peer_comm->remote_group);

      fn_fail:
        /* In case root fails, we bcast mpi_errno so other ranks will abort too */
        MPIR_Bcast_impl(&mpi_errno, 1, MPIR_INT_INTERNAL, root, comm_ptr, MPIR_COLL_ATTR_SYNC);
    } else {
        int root_errno;
        MPIR_Bcast_impl(&root_errno, 1, MPIR_INT_INTERNAL, root, comm_ptr, MPIR_COLL_ATTR_SYNC);
        if (root_errno) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_PORT, "**comm_connect_fail");
        }
    }

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = MPIR_Intercomm_create_timeout(comm_ptr, root, peer_comm, 0, tag, timeout,
                                                  newcomm);
    }

    if (comm_ptr->rank == root && peer_comm) {
        /* destroy peer_comm */
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));
        MPIDIU_free_dynamic_lpid(remote_lpid);
        MPIR_Group_release(peer_comm->remote_group);
        MPIR_Handle_obj_free(&MPIR_Comm_mem, peer_comm);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    }

    return mpi_errno;
}

int MPID_Comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                     MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    int timeout = 0;
    if (info != NULL) {
        int info_flag = 0;
        char info_value[MPI_MAX_INFO_VAL + 1];
        MPIR_Info_get_impl(info, "timeout", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            timeout = atoi(info_value);
        }
    }
    bool is_sender = false;
    mpi_errno = dynamic_intercomm_create(port_name, info, root, comm,
                                         timeout, is_sender, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_connect(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                      MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int timeout = MPIR_CVAR_CH4_COMM_CONNECT_TIMEOUT;
    if (info != NULL) {
        int info_flag = 0;
        char info_value[MPI_MAX_INFO_VAL + 1];
        MPIR_Info_get_impl(info, "timeout", MPI_MAX_INFO_VAL, info_value, &info_flag);
        if (info_flag) {
            timeout = atoi(info_value);
        }
    }
    bool is_sender = true;
    mpi_errno = dynamic_intercomm_create(port_name, info, root, comm,
                                         timeout, is_sender, newcomm_ptr);
    if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIX_ERR_TIMEOUT) {
        /* when connect timeout, a likely reason is the server not ready.
         * By convention, we return MPI_ERR_PORT.
         */
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_PORT, goto fn_fail, "**fail");
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPID_Comm_disconnect */

int MPID_Comm_disconnect(MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    /* TODO: run a barrier? */
    mpi_errno = MPIR_Comm_free_impl(comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
