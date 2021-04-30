/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpidu_bc.h"
#include "ofi_noinline.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : CH4_OFI
      description : A category for CH4 OFI netmod variables

cvars:
    - name        : MPIR_CVAR_CH4_OFI_CAPABILITY_SETS_DEBUG
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Prints out the configuration of each capability selected via the capability sets interface.

    - name        : MPIR_CVAR_OFI_SKIP_IPV6
      category    : DEVELOPER
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Skip IPv6 providers.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, the OFI addressing information will be stored with an FI_AV_TABLE.
        If false, an FI_AV_MAP will be used.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, use OFI scalable endpoints.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to false (zero), MPICH does not use OFI shared contexts.
        If set to -1, it is determined by the OFI capability sets based on the provider.
        Otherwise, MPICH tries to use OFI shared contexts. If they are unavailable,
        it'll fall back to the mode without shared contexts.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_SCALABLE
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        This variable is only provided for backward compatibility. When using OFI versions 1.5+, use
        the other memory region variables.

        If true, MR_SCALABLE for OFI memory regions.
        If false, MR_BASIC for OFI memory regions.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable virtual addressing for OFI memory regions. This variable is only meaningful
        for OFI versions 1.5+. It is equivelent to using FI_MR_BASIC in versions of
        OFI older than 1.5.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_ALLOCATED
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, require all OFI memory regions must be backed by physical memory pages
        at the time the registration call is made. This variable is only meaningful
        for OFI versions 1.5+. It is equivelent to using FI_MR_BASIC in versions of
        OFI older than 1.5.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable provider supplied key for OFI memory regions. This variable is only
        meaningful for OFI versions 1.5+. It is equivelent to using FI_MR_BASIC in versions of OFI
        older than 1.5.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_TAGGED
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, use tagged message transmission functions in OFI.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_AM
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI active message support.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_RMA
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI RMA support for MPI RMA operations. OFI support for basic RMA is always
        required to implement large messgage transfers in the active message code path.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI Atomics support.

    - name        : MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the maximum number of iovecs that can be used by the OFI provider
        for fetch_atomic operations. The default value is -1, indicating that
        no value is set.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_DATA_AUTO_PROGRESS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable MPI data auto progress.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable MPI control auto progress.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable iovec for pt2pt.

    - name        : MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of bits that will be used for matching the context
        ID. The default value is -1, indicating that no value is set and that
        the default will be defined in the ofi_types.h file.

    - name        : MPIR_CVAR_CH4_OFI_RANK_BITS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of bits that will be used for matching the MPI
        rank. The default value is -1, indicating that no value is set and that
        the default will be defined in the ofi_types.h file.

    - name        : MPIR_CVAR_CH4_OFI_TAG_BITS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of bits that will be used for matching the user
        tag. The default value is -1, indicating that no value is set and that
        the default will be defined in the ofi_types.h file.

    - name        : MPIR_CVAR_CH4_OFI_MAJOR_VERSION
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the major version of the OFI library. The default is the
        major version of the OFI library used with MPICH. If using this CVAR,
        it is recommended that the user also specifies a specific OFI provider.

    - name        : MPIR_CVAR_CH4_OFI_MINOR_VERSION
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the major version of the OFI library. The default is the
        minor version of the OFI library used with MPICH. If using this CVAR,
        it is recommended that the user also specifies a specific OFI provider.

    - name        : MPIR_CVAR_CH4_OFI_MAX_VNIS
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of CH4 VNIs
        that OFI netmod exposes. If set to 0 (the default) or bigger than
        MPIR_CVAR_CH4_NUM_VCIS, the number of exposed VNIs is set to MPIR_CVAR_CH4_NUM_VCIS.

    - name        : MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of transmit
        contexts RMA can utilize in a scalable endpoint.
        This value is effective only when scalable endpoint is available, otherwise
        it will be ignored.

    - name        : MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of retries
        of an ofi operations before returning MPIX_ERR_EAGAIN. This value is
        effective only when the communicator has the MPI_OFI_set_eagain info
        hint set to true.

    - name        : MPIR_CVAR_CH4_OFI_NUM_AM_BUFFERS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of buffers for receiving active messages.

    - name        : MPIR_CVAR_CH4_OFI_RMA_PROGRESS_INTERVAL
      category    : CH4_OFI
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the interval for manually flushing RMA operations when automatic progress is not
        enabled. It the underlying OFI provider supports auto data progress, this value is ignored.
        If the value is -1, this optimization will be turned off.

    - name        : MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX
      category    : CH4_OFI
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the maximum number of iovecs to allocate for RMA operations
        to/from noncontiguous buffers.

    - name        : MPIR_CVAR_CH4_OFI_NUM_PACK_BUFFERS_PER_CHUNK
      category    : CH4_OFI
      type        : int
      default     : 16
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of buffers for packing/unpacking messages in
        each block of the pool.

    - name        : MPIR_CVAR_CH4_OFI_MAX_NUM_PACK_BUFFERS
      category    : CH4_OFI
      type        : int
      default     : 256
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the max number of buffers for packing/unpacking messages
        in the pool.

    - name        : MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        This cvar controls the message size at which OFI native path switches from eager to
        rendezvous mode. It does not affect the AM path eager limit. Having this gives a way to
        reliably test native non-path.
        If the number is positive, OFI will init the MPIDI_OFI_global.max_msg_size to the value of
        cvar. If the number is negative, OFI will init the MPIDI_OFI_globa.max_msg_size using
        whatever provider gives (which might be unlimited for socket provider).

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static int get_ofi_version(void);
static int open_fabric(void);
static int create_vni_context(int vni);
static int destroy_vni_context(int vni);

static int conn_manager_init(void);
static int conn_manager_destroy(void);
static int dynproc_send_disconnect(int conn_id);

static int addr_exchange_root_vni(MPIR_Comm * init_comm);
static int addr_exchange_all_vnis(void);

static void *host_alloc(uintptr_t size);
static void *host_alloc_registered(uintptr_t size);
static void host_free(void *ptr);
static void host_free_registered(void *ptr);

static void *host_alloc(uintptr_t size)
{
    return MPL_malloc(size, MPL_MEM_BUFFER);
}

static void *host_alloc_registered(uintptr_t size)
{
    void *ptr = MPL_malloc(size, MPL_MEM_BUFFER);
    MPIR_Assert(ptr);
    MPL_gpu_register_host(ptr, size);
    return ptr;
}

static void host_free(void *ptr)
{
    MPL_free(ptr);
}

static void host_free_registered(void *ptr)
{
    MPL_gpu_unregister_host(ptr);
    MPL_free(ptr);
}

static int get_ofi_version(void)
{
    if (MPIDI_OFI_MAJOR_VERSION != -1 && MPIDI_OFI_MINOR_VERSION != -1)
        return FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION);
    else
        return FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION);
}

static int conn_manager_init()
{
    int mpi_errno = MPI_SUCCESS, i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_INIT);

    MPIDI_OFI_global.conn_mgr.max_n_conn = 1;
    MPIDI_OFI_global.conn_mgr.next_conn_id = 0;
    MPIDI_OFI_global.conn_mgr.n_conn = 0;

    MPIDI_OFI_global.conn_mgr.conn_list =
        (MPIDI_OFI_conn_t *) MPL_malloc(8 * 4 * 1024 /* FIXME: what is this size? */ ,
                                        MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_OFI_global.conn_mgr.conn_list == NULL, mpi_errno, MPI_ERR_NO_MEM,
                        goto fn_fail, "**nomem");

    MPIDI_OFI_global.conn_mgr.free_conn_id =
        (int *) MPL_malloc(MPIDI_OFI_global.conn_mgr.max_n_conn * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_OFI_global.conn_mgr.free_conn_id == NULL, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    for (i = 0; i < MPIDI_OFI_global.conn_mgr.max_n_conn; ++i) {
        MPIDI_OFI_global.conn_mgr.free_conn_id[i] = i + 1;
    }
    MPIDI_OFI_global.conn_mgr.free_conn_id[MPIDI_OFI_global.conn_mgr.max_n_conn - 1] = -1;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int conn_manager_destroy()
{
    int mpi_errno = MPI_SUCCESS, i, j;
    MPIDI_OFI_dynamic_process_request_t *req;
    fi_addr_t *conn;
    int max_n_conn = MPIDI_OFI_global.conn_mgr.max_n_conn;
    int *close_msg;
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    MPIR_Context_id_t context_id = 0xF000;
    MPIR_CHKLMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_DESTROY);

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, 1);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    if (max_n_conn > 0) {
        /* try wait/close connections */
        MPIR_CHKLMEM_MALLOC(req, MPIDI_OFI_dynamic_process_request_t *,
                            max_n_conn * sizeof(MPIDI_OFI_dynamic_process_request_t), mpi_errno,
                            "req", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(conn, fi_addr_t *, max_n_conn * sizeof(fi_addr_t), mpi_errno, "conn",
                            MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(close_msg, int *, max_n_conn * sizeof(int), mpi_errno, "int",
                            MPL_MEM_BUFFER);

        j = 0;
        for (i = 0; i < max_n_conn; ++i) {
            switch (MPIDI_OFI_global.conn_mgr.conn_list[i].state) {
                case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
                    mpi_errno = dynproc_send_disconnect(i);
                    MPIR_ERR_CHECK(mpi_errno);
                    break;
                case MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT:
                case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
                    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                    (MPL_DBG_FDEST, "Wait for close of conn_id=%d", i));
                    conn[j] = MPIDI_OFI_global.conn_mgr.conn_list[i].dest;
                    req[j].done = 0;
                    req[j].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
                    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[0].rx,
                                                  &close_msg[j],
                                                  sizeof(int),
                                                  NULL,
                                                  conn[j],
                                                  match_bits,
                                                  mask_bits, &req[j].context), 0, trecv, FALSE);
                    j++;
                    break;
                default:
                    break;
            }
        }

        for (i = 0; i < j; ++i) {
            MPIDI_OFI_PROGRESS_WHILE(!req[i].done, 0);
            MPIDI_OFI_global.conn_mgr.conn_list[i].state = MPIDI_OFI_DYNPROC_DISCONNECTED;
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST, "conn_id=%d closed", i));
        }

        MPIR_CHKLMEM_FREEALL();
    }

    MPL_free(MPIDI_OFI_global.conn_mgr.conn_list);
    MPL_free(MPIDI_OFI_global.conn_mgr.free_conn_id);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_DESTROY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynproc_send_disconnect(int conn_id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Context_id_t context_id = 0xF000;
    MPIDI_OFI_dynamic_process_request_t req;
    uint64_t match_bits = 0;
    unsigned int close_msg = 0xcccccccc;
    struct fi_msg_tagged msg;
    struct iovec msg_iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DYNPROC_SEND_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DYNPROC_SEND_DISCONNECT);

    if (MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state == MPIDI_OFI_DYNPROC_CONNECTED_CHILD) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, " send disconnect msg conn_id=%d from child side",
                         conn_id));
        match_bits = MPIDI_OFI_init_sendtag(context_id, 1, MPIDI_OFI_DYNPROC_SEND);

        /* fi_av_map here is not quite right for some providers */
        /* we need to get this connection from the sockname     */
        req.done = 0;
        req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        msg_iov.iov_base = &close_msg;
        msg_iov.iov_len = sizeof(close_msg);
        msg.msg_iov = &msg_iov;
        msg.desc = NULL;
        msg.iov_count = 0;
        msg.addr = MPIDI_OFI_global.conn_mgr.conn_list[conn_id].dest;
        msg.tag = match_bits;
        msg.ignore = context_id;
        msg.context = (void *) &req.context;
        msg.data = 0;
        MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[0].tx, &msg,
                                         FI_COMPLETION | FI_TRANSMIT_COMPLETE | FI_REMOTE_CQ_DATA),
                             0, tsendmsg, FALSE);
        MPIDI_OFI_PROGRESS_WHILE(!req.done, 0);
    }

    switch (MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state) {
        case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
            MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state =
                MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_CHILD;
            break;
        case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
            MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state =
                MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT;
            break;
        default:
            break;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " local_disconnected conn_id=%d state=%d",
                     conn_id, MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DYNPROC_SEND_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_init_hook(int rank, int size, int appnum, int *tag_bits, MPIR_Comm * init_comm)
{
    int mpi_errno = MPI_SUCCESS, i;
    size_t optlen;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_INIT_HOOK);

    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_chunk_request, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_huge_recv_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_am_repost_request_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_ssendack_request_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_dynamic_process_request_t, context));
    MPL_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.am.netmod_am.ofi.context) ==
                            offsetof(struct MPIR_Request, dev.ch4.netmod.ofi.context));
    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devreq_t) >= sizeof(MPIDI_OFI_request_t));

    int err;
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_UTIL_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_FI_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &err);
    MPIR_Assert(err == 0);

    mpi_errno = open_fabric();
    MPIR_ERR_CHECK(mpi_errno);

    /* ------------------------------------------------------------------------ */
    /* Create transport level communication contexts.                           */
    /* ------------------------------------------------------------------------ */

    int num_vnis = 1;
    if (MPIR_CVAR_CH4_OFI_MAX_VNIS == 0 || MPIR_CVAR_CH4_OFI_MAX_VNIS > MPIDI_global.n_vcis) {
        num_vnis = MPIDI_global.n_vcis;
    } else {
        num_vnis = MPIR_CVAR_CH4_OFI_MAX_VNIS;
    }

    /* TODO: update num_vnis according to provider capabilities, such as
     * prov_use->domain_attr->{tx,rx}_ctx_cnt
     */
    if (num_vnis > MPIDI_OFI_MAX_VNIS) {
        num_vnis = MPIDI_OFI_MAX_VNIS;
    }
    /* for best performance, we ensure 1-to-1 vci/vni mapping. ref: MPIDI_OFI_vci_to_vni */
    /* TODO: allow less num_vnis. Option 1. runtime MOD; 2. overide MPIDI_global.n_vcis */
    MPIR_Assert(num_vnis == MPIDI_global.n_vcis);

    /* Multiple vni without using domain require MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS */
#ifndef MPIDI_OFI_VNI_USE_DOMAIN
    MPIR_Assert(num_vnis == 1 || MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
#endif

    /* WorkQ only works with single vni for now */
#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPIR_Assert(num_vnis == 1);
#endif

    MPIDI_OFI_global.num_vnis = num_vnis;

    /* Create MPIDI_OFI_global.ctx[0] first  */
    mpi_errno = create_vni_context(0);
    MPIR_ERR_CHECK(mpi_errno);

    /* Creating the additional vni contexts.
     * This code maybe moved to a later stage */
    for (i = 1; i < MPIDI_OFI_global.num_vnis; i++) {
        mpi_errno = create_vni_context(i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* ------------------------------------------------------------------------ */
    /* Address exchange (essentially activating the vnis)                       */
    /* ------------------------------------------------------------------------ */

    if (!MPIDI_OFI_global.got_named_av) {
        mpi_errno = addr_exchange_root_vni(init_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* -------------------------------- */
    /* Create the id to object maps     */
    /* -------------------------------- */
    MPIDIU_map_create(&MPIDI_OFI_global.win_map, MPL_MEM_RMA);
    MPIDIU_map_create(&MPIDI_OFI_global.req_map, MPL_MEM_OTHER);

    /* ---------------------------------- */
    /* Initialize Active Message          */
    /* ---------------------------------- */
    if (MPIDI_OFI_ENABLE_AM) {
        /* Maximum possible message size for short message send (=eager send)
         * See MPIDI_OFI_do_am_isend for short/long switching logic */
        MPIR_Assert(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE <= MPIDI_OFI_global.max_msg_size);
        MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_OFI_am_request_header_t)
                                < MPIDI_OFI_AM_HDR_POOL_CELL_SIZE);
        MPL_COMPILE_TIME_ASSERT(MPIDI_OFI_AM_HDR_POOL_CELL_SIZE
                                >= sizeof(MPIDI_OFI_am_send_pipeline_request_t));
        mpi_errno =
            MPIDU_genq_private_pool_create_unsafe(MPIDI_OFI_AM_HDR_POOL_CELL_SIZE,
                                                  MPIDI_OFI_AM_HDR_POOL_NUM_CELLS_PER_CHUNK,
                                                  MPIDI_OFI_AM_HDR_POOL_MAX_NUM_CELLS,
                                                  host_alloc, host_free,
                                                  &MPIDI_OFI_global.am_hdr_buf_pool);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_OFI_global.cq_buffered_dynamic_head = MPIDI_OFI_global.cq_buffered_dynamic_tail =
            NULL;
        MPIDI_OFI_global.cq_buffered_static_head = MPIDI_OFI_global.cq_buffered_static_tail = 0;
        optlen = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;

        MPIDI_OFI_CALL(fi_setopt(&(MPIDI_OFI_global.ctx[0].rx->fid),
                                 FI_OPT_ENDPOINT,
                                 FI_OPT_MIN_MULTI_RECV, &optlen, sizeof(optlen)), setopt);

        MPIDIU_map_create(&MPIDI_OFI_global.am_recv_seq_tracker, MPL_MEM_BUFFER);
        MPIDIU_map_create(&MPIDI_OFI_global.am_send_seq_tracker, MPL_MEM_BUFFER);
        MPIDI_OFI_global.am_unordered_msgs = NULL;

        for (i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++) {
            MPL_gpu_malloc_host(&(MPIDI_OFI_global.am_bufs[i]), MPIDI_OFI_AM_BUFF_SZ);
            MPIDI_OFI_global.am_reqs[i].event_id = MPIDI_OFI_EVENT_AM_RECV;
            MPIDI_OFI_global.am_reqs[i].index = i;
            MPIR_Assert(MPIDI_OFI_global.am_bufs[i]);
            MPIDI_OFI_global.am_iov[i].iov_base = MPIDI_OFI_global.am_bufs[i];
            MPIDI_OFI_global.am_iov[i].iov_len = MPIDI_OFI_AM_BUFF_SZ;
            MPIDI_OFI_global.am_msg[i].msg_iov = &MPIDI_OFI_global.am_iov[i];
            MPIDI_OFI_global.am_msg[i].desc = NULL;
            MPIDI_OFI_global.am_msg[i].addr = FI_ADDR_UNSPEC;
            MPIDI_OFI_global.am_msg[i].context = &MPIDI_OFI_global.am_reqs[i].context;
            MPIDI_OFI_global.am_msg[i].iov_count = 1;
            MPIDI_OFI_CALL_RETRY(fi_recvmsg(MPIDI_OFI_global.ctx[0].rx,
                                            &MPIDI_OFI_global.am_msg[i],
                                            FI_MULTI_RECV | FI_COMPLETION), 0, prepost, FALSE);
        }

        MPIDIG_am_reg_cb(MPIDI_OFI_INTERNAL_HANDLER_CONTROL, NULL, &MPIDI_OFI_control_handler);
    }
    MPL_atomic_store_int(&MPIDI_OFI_global.am_inflight_inject_emus, 0);
    MPL_atomic_store_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs, 0);

    /* Create pack buffer pool */
    mpi_errno =
        MPIDU_genq_private_pool_create_unsafe(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE,
                                              MPIR_CVAR_CH4_OFI_NUM_PACK_BUFFERS_PER_CHUNK,
                                              MPIR_CVAR_CH4_OFI_MAX_NUM_PACK_BUFFERS,
                                              host_alloc_registered,
                                              host_free_registered,
                                              &MPIDI_OFI_global.pack_buf_pool);
    MPIR_ERR_CHECK(mpi_errno);

    /* Initalize RMA keys allocator */
    MPIDI_OFI_mr_key_allocator_init();

    /* ------------------------------------------------- */
    /* Initialize Connection Manager for Dynamic Tasking */
    /* ------------------------------------------------- */
    conn_manager_init();

    MPIR_Comm_register_hint(MPIR_COMM_HINT_EAGAIN, "eagain", NULL, MPIR_COMM_HINT_TYPE_BOOL, 0);

    /* index datatypes for RMA atomics */
    MPIDI_OFI_index_datatypes();

    MPIDI_OFI_global.deferred_am_isend_q = NULL;

  fn_exit:
    *tag_bits = MPIDI_OFI_TAG_BITS;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i = 0;
    int barrier[2] = { 0 };
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_FINALIZE_HOOK);

    /* clean dynamic process connections */
    mpi_errno = conn_manager_destroy();
    MPIR_ERR_CHECK(mpi_errno);

    /* Progress until we drain all inflight RMA send long buffers */
    /* NOTE: am currently only use vni 0. Need update once that changes */
    while (MPL_atomic_load_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs) > 0)
        MPIDI_OFI_PROGRESS(0);

    /* Destroy RMA key allocator */
    MPIDI_OFI_mr_key_allocator_destroy();

    /* Barrier over allreduce, but force non-immediate send */
    MPIDI_OFI_global.max_buffered_send = 0;
    mpi_errno = MPIR_Allreduce_allcomm_auto(&barrier[0], &barrier[1], 1, MPI_INT, MPI_SUM,
                                            MPIR_Process.comm_world, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* Progress until we drain all inflight injection emulation requests */
    /* NOTE: am currently only use vni 0. Need update once that changes */
    while (MPL_atomic_load_int(&MPIDI_OFI_global.am_inflight_inject_emus) > 0)
        MPIDI_OFI_PROGRESS(0);
    MPIR_Assert(MPL_atomic_load_int(&MPIDI_OFI_global.am_inflight_inject_emus) == 0);

    /* Tearing down endpoints */
    for (i = 1; i < MPIDI_OFI_global.num_vnis; i++) {
        mpi_errno = destroy_vni_context(i);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* 0th ctx is special, synonymous to global context */
    mpi_errno = destroy_vni_context(0);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.fabric->fid), fabricclose);

    fi_freeinfo(MPIDI_OFI_global.prov_use);

    MPIDIU_map_destroy(MPIDI_OFI_global.win_map);
    MPIDIU_map_destroy(MPIDI_OFI_global.req_map);

    if (MPIDI_OFI_ENABLE_AM) {
        while (MPIDI_OFI_global.am_unordered_msgs) {
            MPIDI_OFI_am_unordered_msg_t *uo_msg = MPIDI_OFI_global.am_unordered_msgs;
            DL_DELETE(MPIDI_OFI_global.am_unordered_msgs, uo_msg);
        }
        MPIDIU_map_destroy(MPIDI_OFI_global.am_send_seq_tracker);
        MPIDIU_map_destroy(MPIDI_OFI_global.am_recv_seq_tracker);

        for (i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++)
            MPL_gpu_free_host(MPIDI_OFI_global.am_bufs[i]);

        MPIDU_genq_private_pool_destroy_unsafe(MPIDI_OFI_global.am_hdr_buf_pool);

        MPIR_Assert(MPIDI_OFI_global.cq_buffered_static_head ==
                    MPIDI_OFI_global.cq_buffered_static_tail);
        MPIR_Assert(NULL == MPIDI_OFI_global.cq_buffered_dynamic_head);
    }

    MPIDU_genq_private_pool_destroy_unsafe(MPIDI_OFI_global.pack_buf_pool);

    int err;
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_UTIL_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_FI_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &err);
    MPIR_Assert(err == 0);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    if (MPIDI_OFI_global.num_vnis > 1) {
        mpi_errno = addr_exchange_all_vnis();
    }
    return mpi_errno;
}

int MPIDI_OFI_get_vci_attr(int vci)
{
    MPIR_Assert(0 <= vci && vci < 1);
    return MPIDI_VCI_TX | MPIDI_VCI_RX;
}

void *MPIDI_OFI_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}

int MPIDI_OFI_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}

/* ---- static functions for vni contexts ---- */
static int create_vni_domain(struct fid_domain **p_domain, struct fid_av **p_av,
                             struct fid_cntr **p_cntr);
static int create_cq(struct fid_domain *domain, struct fid_cq **p_cq);
static int create_sep_tx(struct fid_ep *ep, int idx, struct fid_ep **p_tx,
                         struct fid_cq *cq, struct fid_cntr *cntr);
static int create_sep_rx(struct fid_ep *ep, int idx, struct fid_ep **p_rx,
                         struct fid_cq *cq, struct fid_cntr *cntr);
static int try_open_shared_av(struct fid_domain *domain, struct fid_av **p_av);
static int create_rma_stx_ctx(struct fid_domain *domain, struct fid_stx **p_rma_stx_ctx);

static int create_vni_context(int vni)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CREATE_VNI_CONTEXT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CREATE_VNI_CONTEXT);

    struct fi_info *prov_use = MPIDI_OFI_global.prov_use;

    /* Each VNI context consists of domain, av, cq, cntr, etc.
     *
     * If MPIDI_OFI_VNI_USE_DOMAIN is true, each context is a separate domain,
     * within which are each separate av, cq, cntr, ..., everything. Within the
     * VNI context, it still can use either simple endpoint or scalable endpoint.
     *
     * If MPIDI_OFI_VNI_USE_DOMAIN is false, then all the VNI contexts will share
     * the same domain and av, and use a single scalable endpoint. Separate VNI
     * context will have its separate cq and separate tx and rx with the SEP.
     *
     * To accomodate both configurations, each context structure will have all fields
     * including domain, av, cq, ... For "VNI_USE_DOMAIN", they are not shared.
     * When not "VNI_USE_DOMAIN" or "VNI_USE_SEPCTX", domain, av, and ep are shared
     * with the root (or 0th) VNI context.
     */
    struct fid_domain *domain;
    struct fid_av *av;
    struct fid_cntr *rma_cmpl_cntr;
    struct fid_cq *cq;

    struct fid_ep *ep;
    struct fid_ep *tx;
    struct fid_ep *rx;

#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    mpi_errno = create_vni_domain(&domain, &av, &rma_cmpl_cntr);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = create_cq(domain, &cq);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_CALL(fi_scalable_ep(domain, prov_use, &ep, NULL), ep);
        MPIDI_OFI_CALL(fi_scalable_ep_bind(ep, &av->fid, 0), bind);
        MPIDI_OFI_CALL(fi_enable(ep), ep_enable);

        mpi_errno = create_sep_tx(ep, 0, &tx, cq, rma_cmpl_cntr);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = create_sep_rx(ep, 0, &rx, cq, rma_cmpl_cntr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIDI_OFI_CALL(fi_endpoint(domain, prov_use, &ep, NULL), ep);
        MPIDI_OFI_CALL(fi_ep_bind(ep, &av->fid, 0), bind);
        MPIDI_OFI_CALL(fi_ep_bind(ep, &cq->fid, FI_SEND | FI_RECV | FI_SELECTIVE_COMPLETION), bind);
        MPIDI_OFI_CALL(fi_ep_bind(ep, &rma_cmpl_cntr->fid, FI_READ | FI_WRITE), bind);
        MPIDI_OFI_CALL(fi_enable(ep), ep_enable);
        tx = ep;
        rx = ep;
    }
    MPIDI_OFI_global.ctx[vni].domain = domain;
    MPIDI_OFI_global.ctx[vni].av = av;
    MPIDI_OFI_global.ctx[vni].rma_cmpl_cntr = rma_cmpl_cntr;
    MPIDI_OFI_global.ctx[vni].ep = ep;
    MPIDI_OFI_global.ctx[vni].cq = cq;
    MPIDI_OFI_global.ctx[vni].tx = tx;
    MPIDI_OFI_global.ctx[vni].rx = rx;

#else /* MPIDI_OFI_VNI_USE_SEPCTX */
    if (vni == 0) {
        mpi_errno = create_vni_domain(&domain, &av, &rma_cmpl_cntr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        domain = MPIDI_OFI_global.ctx[0].domain;
        av = MPIDI_OFI_global.ctx[0].av;
        rma_cmpl_cntr = MPIDI_OFI_global.ctx[0].rma_cmpl_cntr;
    }
    mpi_errno = create_cq(domain, &cq);
    MPIR_ERR_CHECK(mpi_errno);

    if (vni == 0) {
        if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
            MPIDI_OFI_CALL(fi_scalable_ep(domain, prov_use, &ep, NULL), ep);
            MPIDI_OFI_CALL(fi_scalable_ep_bind(ep, &av->fid, 0), bind);
            MPIDI_OFI_CALL(fi_enable(ep), ep_enable);
        } else {
            MPIDI_OFI_CALL(fi_endpoint(domain, prov_use, &ep, NULL), ep);
            MPIDI_OFI_CALL(fi_ep_bind(ep, &av->fid, 0), bind);
            MPIDI_OFI_CALL(fi_ep_bind(ep, &cq->fid, FI_SEND | FI_RECV | FI_SELECTIVE_COMPLETION),
                           bind);
            MPIDI_OFI_CALL(fi_ep_bind(ep, &rma_cmpl_cntr->fid, FI_READ | FI_WRITE), bind);
            MPIDI_OFI_CALL(fi_enable(ep), ep_enable);
        }
    } else {
        ep = MPIDI_OFI_global.ctx[0].ep;
    }

    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        mpi_errno = create_sep_tx(ep, vni, &tx, cq, rma_cmpl_cntr);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = create_sep_rx(ep, vni, &rx, cq, rma_cmpl_cntr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        tx = ep;
        rx = ep;
    }

    if (vni == 0) {
        MPIDI_OFI_global.ctx[0].domain = domain;
        MPIDI_OFI_global.ctx[0].av = av;
        MPIDI_OFI_global.ctx[0].rma_cmpl_cntr = rma_cmpl_cntr;
        MPIDI_OFI_global.ctx[0].ep = ep;
    }
    MPIDI_OFI_global.ctx[vni].cq = cq;
    MPIDI_OFI_global.ctx[vni].tx = tx;
    MPIDI_OFI_global.ctx[vni].rx = rx;
#endif

    /* ------------------------------------------------------------------------ */
    /* Construct:  Shared TX Context for RMA                                    */
    /* ------------------------------------------------------------------------ */
    if (vni == 0) {
        mpi_errno = create_rma_stx_ctx(domain, &MPIDI_OFI_global.rma_stx_ctx);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CREATE_VNI_CONTEXT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---------------------------------------------------------- */
/* Provider Selections and open_fabric()                      */
/* ---------------------------------------------------------- */
static int find_provider(struct fi_info *hints);
static void update_global_settings(struct fi_info *prov, struct fi_info *hints);
static void dump_global_settings(void);

/* set MPIDI_OFI_global.settings based on provider-set */
static void init_global_settings(const char *prov_name);
/* set hints based on MPIDI_OFI_global.settings */
static void init_hints(struct fi_info *hints);
/* whether prov matches MPIDI_OFI_global.settings */
bool match_global_settings(struct fi_info *prov);
/* picks one matching provider from the list or return NULL */
static struct fi_info *pick_provider_from_list(const char *provname, struct fi_info *prov_list);
static struct fi_info *pick_provider_by_name(const char *provname, struct fi_info *prov_list);
static struct fi_info *pick_provider_by_global_settings(struct fi_info *prov_list);

static int destroy_vni_context(int vni)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_global.ctx[vni].tx), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_global.ctx[vni].rx), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_global.ctx[vni].cq), cqclose);

        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].ep->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].av->fid), avclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].rma_cmpl_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].domain->fid), domainclose);
    } else {    /* normal endpoint */
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].ep->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].cq->fid), cqclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].av->fid), avclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].rma_cmpl_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].domain->fid), domainclose);
    }

#else /* MPIDI_OFI_VNI_USE_SEPCTX */
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_global.ctx[vni].tx), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_global.ctx[vni].rx), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_global.ctx[vni].cq), cqclose);
        if (vni == 0) {
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].ep->fid), epclose);
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].av->fid), avclose);
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].rma_cmpl_cntr->fid), cntrclose);
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].domain->fid), domainclose);
        }
    } else {    /* normal endpoint */
        MPIR_Assert(vni == 0);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].ep->fid), epclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].cq->fid), cqclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].av->fid), avclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].rma_cmpl_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.ctx[vni].domain->fid), domainclose);
    }
#endif
    if (vni == 0) {
        /* Close RMA scalable EP. */
        if (MPIDI_OFI_global.rma_sep) {
            /* All transmit contexts on RMA must be closed. */
            MPIR_Assert(utarray_len(MPIDI_OFI_global.rma_sep_idx_array) ==
                        MPIDI_OFI_global.max_rma_sep_tx_cnt);
            utarray_free(MPIDI_OFI_global.rma_sep_idx_array);
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.rma_sep->fid), epclose);
        }

        if (MPIDI_OFI_global.rma_stx_ctx != NULL) {
            MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.rma_stx_ctx->fid), stx_ctx_close);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DESTROY_VNI_CONTEXT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_vni_domain(struct fid_domain **p_domain, struct fid_av **p_av,
                             struct fid_cntr **p_cntr)
{
    int mpi_errno = MPI_SUCCESS;

    /* ---- domain ---- */
    struct fid_domain *domain;
    MPIDI_OFI_CALL(fi_domain(MPIDI_OFI_global.fabric, MPIDI_OFI_global.prov_use, &domain, NULL),
                   opendomain);
    *p_domain = domain;

    /* ---- av ---- */
    /* ----
     * Attempt to open a shared address vector read-only.
     * The open will fail if the address vector does not exist.
     * Otherwise, set MPIDI_OFI_global.got_named_av and
     * copy the map_addr.
     */
    if (try_open_shared_av(domain, p_av)) {
        MPIDI_OFI_global.got_named_av = 1;
    }

    if (!MPIDI_OFI_global.got_named_av) {
        struct fi_av_attr av_attr;
        memset(&av_attr, 0, sizeof(av_attr));
        if (MPIDI_OFI_ENABLE_AV_TABLE) {
            av_attr.type = FI_AV_TABLE;
        } else {
            av_attr.type = FI_AV_MAP;
        }
        av_attr.rx_ctx_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS;
        av_attr.count = MPIR_Process.size;

        av_attr.name = NULL;
        av_attr.flags = 0;
        MPIDI_OFI_CALL(fi_av_open(domain, &av_attr, p_av, NULL), avopen);
    }

    /* ---- other sharable objects ---- */
    struct fi_cntr_attr cntr_attr;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    cntr_attr.events = FI_CNTR_EVENTS_COMP;
    cntr_attr.wait_obj = FI_WAIT_UNSPEC;
    MPIDI_OFI_CALL(fi_cntr_open(domain, &cntr_attr, p_cntr, NULL), openct);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_cq(struct fid_domain *domain, struct fid_cq **p_cq)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_attr cq_attr;
    memset(&cq_attr, 0, sizeof(cq_attr));
    cq_attr.format = FI_CQ_FORMAT_TAGGED;
    MPIDI_OFI_CALL(fi_cq_open(domain, &cq_attr, p_cq, NULL), opencq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_sep_tx(struct fid_ep *ep, int idx, struct fid_ep **p_tx,
                         struct fid_cq *cq, struct fid_cntr *cntr)
{
    int mpi_errno = MPI_SUCCESS;

    struct fi_tx_attr tx_attr;
    tx_attr = *(MPIDI_OFI_global.prov_use->tx_attr);
    tx_attr.op_flags = FI_COMPLETION;
    if (MPIDI_OFI_ENABLE_RMA || MPIDI_OFI_ENABLE_ATOMICS)
        tx_attr.op_flags |= FI_DELIVERY_COMPLETE;
    tx_attr.caps = 0;

    if (MPIDI_OFI_ENABLE_TAGGED)
        tx_attr.caps = FI_TAGGED;

    /* RMA */
    if (MPIDI_OFI_ENABLE_RMA)
        tx_attr.caps |= FI_RMA;
    if (MPIDI_OFI_ENABLE_ATOMICS)
        tx_attr.caps |= FI_ATOMICS;
    /* MSG */
    tx_attr.caps |= FI_MSG;
    tx_attr.caps |= FI_NAMED_RX_CTX;    /* Required for scalable endpoints indexing */

    MPIDI_OFI_CALL(fi_tx_context(ep, idx, &tx_attr, p_tx, NULL), ep);
    MPIDI_OFI_CALL(fi_ep_bind(*p_tx, &cq->fid, FI_SEND | FI_SELECTIVE_COMPLETION), bind);
    MPIDI_OFI_CALL(fi_ep_bind(*p_tx, &cntr->fid, FI_WRITE | FI_READ), bind);
    MPIDI_OFI_CALL(fi_enable(*p_tx), ep_enable);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_sep_rx(struct fid_ep *ep, int idx, struct fid_ep **p_rx,
                         struct fid_cq *cq, struct fid_cntr *cntr)
{
    int mpi_errno = MPI_SUCCESS;

    struct fi_rx_attr rx_attr;
    rx_attr = *(MPIDI_OFI_global.prov_use->rx_attr);
    rx_attr.caps = 0;

    if (MPIDI_OFI_ENABLE_TAGGED) {
        rx_attr.caps |= FI_TAGGED;
        rx_attr.caps |= FI_DIRECTED_RECV;
    }

    if (MPIDI_OFI_ENABLE_RMA)
        rx_attr.caps |= FI_RMA | FI_REMOTE_READ | FI_REMOTE_WRITE;
    if (MPIDI_OFI_ENABLE_ATOMICS)
        rx_attr.caps |= FI_ATOMICS;
    rx_attr.caps |= FI_MSG;
    rx_attr.caps |= FI_MULTI_RECV;
    rx_attr.caps |= FI_NAMED_RX_CTX;    /* Required for scalable endpoints indexing */

    MPIDI_OFI_CALL(fi_rx_context(ep, idx, &rx_attr, p_rx, NULL), ep);
    MPIDI_OFI_CALL(fi_ep_bind(*p_rx, &cq->fid, FI_RECV), bind);
    MPIDI_OFI_CALL(fi_enable(*p_rx), ep_enable);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int try_open_shared_av(struct fid_domain *domain, struct fid_av **p_av)
{
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    /* shared/named av table cannot be used when multiple fi_domain is enabled */
    return 0;
#else
    struct fi_av_attr av_attr;
    memset(&av_attr, 0, sizeof(av_attr));
    if (MPIDI_OFI_ENABLE_AV_TABLE) {
        av_attr.type = FI_AV_TABLE;
    } else {
        av_attr.type = FI_AV_MAP;
    }
    av_attr.rx_ctx_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS;
    av_attr.count = MPIR_Process.size;

    char av_name[128];
    MPL_snprintf(av_name, sizeof(av_name), "FI_NAMED_AV_%d\n", MPIR_Process.appnum);
    av_attr.name = av_name;
    av_attr.flags = FI_READ;
    av_attr.map_addr = 0;

    if (0 == fi_av_open(domain, &av_attr, p_av, NULL)) {
        /* TODO - the copy from the pre-existing av map into the 'MPIDI_OFI_AV' */
        /* is wasteful and should be changed so that the 'MPIDI_OFI_AV' object  */
        /* directly references the mapped fi_addr_t array instead               */
        fi_addr_t *mapped_table = (fi_addr_t *) av_attr.map_addr;
        for (int i = 0; i < MPIR_Process.size; i++) {
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest[0][0] = mapped_table[i];
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " grank mapped to: rank=%d, av=%p, dest=%" PRIu64,
                             i, (void *) &MPIDIU_get_av(0, i), mapped_table[i]));
        }
        return 1;
    } else {
        return 0;
    }
#endif
}

static int create_rma_stx_ctx(struct fid_domain *domain, struct fid_stx **p_rma_stx_ctx)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_OFI_ENABLE_SHARED_CONTEXTS) {
        int ret;
        struct fi_tx_attr tx_attr;
        memset(&tx_attr, 0, sizeof(tx_attr));
        /* A shared transmit context’s attributes must be a union of all associated
         * endpoints' transmit capabilities. */
        tx_attr.caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;
        tx_attr.msg_order = FI_ORDER_RAR | FI_ORDER_RAW | FI_ORDER_WAR | FI_ORDER_WAW;
        tx_attr.op_flags = FI_DELIVERY_COMPLETE | FI_COMPLETION;
        MPIDI_OFI_CALL_RETURN(fi_stx_context(domain, &tx_attr, p_rma_stx_ctx, NULL), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to create shared TX context for RMA, "
                        "falling back to global EP/counter scheme");
            *p_rma_stx_ctx = NULL;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int open_fabric(void)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_info *prov_list = NULL;

    /* First, find the provider and prepare the hints */
    struct fi_info *hints = fi_allocinfo();
    MPIR_Assert(hints != NULL);

    mpi_errno = find_provider(hints);
    MPIR_ERR_CHECK(mpi_errno);

    /* Second, get the actual fi_info * prov */
    MPIDI_OFI_CALL(fi_getinfo(get_ofi_version(), NULL, NULL, 0ULL, hints, &prov_list), getinfo);

    struct fi_info *prov = prov_list;
    /* fi_getinfo may ignore the addr_format in hints, filter it again */
    if (hints->addr_format != FI_FORMAT_UNSPEC) {
        while (prov && prov->addr_format != hints->addr_format) {
            prov = prov->next;
        }
    }
    MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");
    if (!MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        int set_number = MPIDI_OFI_get_set_number(prov->fabric_attr->prov_name);
        MPIR_ERR_CHKANDJUMP(MPIDI_OFI_SET_NUMBER != set_number,
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
    }

    /* Third, update global settings */
    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        update_global_settings(prov, hints);
    }

    MPIDI_OFI_global.prov_use = fi_dupinfo(prov);
    MPIR_Assert(MPIDI_OFI_global.prov_use);

    MPIDI_OFI_global.max_buffered_send = prov->tx_attr->inject_size;
    MPIDI_OFI_global.max_buffered_write = prov->tx_attr->inject_size;
    if (MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE > 0 &&
        MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE <= prov->ep_attr->max_msg_size) {
        /* Truncate max_msg_size to a user-selected value */
        MPIDI_OFI_global.max_msg_size = MPIR_CVAR_CH4_OFI_EAGER_MAX_MSG_SIZE;
    } else {
        MPIDI_OFI_global.max_msg_size = MPL_MIN(prov->ep_attr->max_msg_size, MPIR_AINT_MAX);
    }
    MPIDI_OFI_global.max_order_raw = prov->ep_attr->max_order_raw_size;
    MPIDI_OFI_global.max_order_war = prov->ep_attr->max_order_war_size;
    MPIDI_OFI_global.max_order_waw = prov->ep_attr->max_order_waw_size;
    MPIDI_OFI_global.tx_iov_limit = MIN(prov->tx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.rx_iov_limit = MIN(prov->rx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.rma_iov_limit = MIN(prov->tx_attr->rma_iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.max_mr_key_size = prov->domain_attr->mr_key_size;

    /* if using extended context id, check that selected provider can support it */
    MPIR_Assert(MPIR_CONTEXT_ID_BITS <= MPIDI_OFI_CONTEXT_BITS);
    /* Check that the desired number of ranks is possible and abort if not */
    if (MPIDI_OFI_MAX_RANK_BITS < 32 && MPIR_Process.size > (1 << MPIDI_OFI_MAX_RANK_BITS)) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch4|too_many_ranks");
    }

    if (MPIR_CVAR_CH4_OFI_CAPABILITY_SETS_DEBUG && MPIR_Process.rank == 0) {
        dump_global_settings();
    }

    /* Finally open the fabric */
    MPIDI_OFI_CALL(fi_fabric(prov->fabric_attr, &MPIDI_OFI_global.fabric, NULL), fabric);

  fn_exit:
    if (prov_list) {
        fi_freeinfo(prov_list);
    }

    /* prov_name is from MPL_strdup, can't let fi_freeinfo to free it */
    MPL_free(hints->fabric_attr->prov_name);
    hints->fabric_attr->prov_name = NULL;
    fi_freeinfo(hints);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int find_provider(struct fi_info *hints)
{
    int mpi_errno = MPI_SUCCESS;

    const char *provname = MPIR_CVAR_OFI_USE_PROVIDER;
    int ofi_version = get_ofi_version();

    if (!MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        init_global_settings(MPIR_CVAR_OFI_USE_PROVIDER);
    } else {
        init_global_settings(MPIR_CVAR_OFI_USE_PROVIDER ? MPIR_CVAR_OFI_USE_PROVIDER :
                             MPIDI_OFI_SET_NAME_DEFAULT);
    }

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        /* Ensure that we aren't trying to shove too many bits into the match_bits.
         * Currently, this needs to fit into a uint64_t and we take 4 bits for protocol. */
        MPIR_Assert(MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS <= 60);

        struct fi_info *prov_list, *prov_use;
        MPIDI_OFI_CALL(fi_getinfo(ofi_version, NULL, NULL, 0ULL, NULL, &prov_list), getinfo);

        prov_use = pick_provider_from_list(provname, prov_list);

        MPIR_ERR_CHKANDJUMP(prov_use == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_getinfo");

        /* Initialize hints based on MPIDI_OFI_global.settings (updated by pick_provider_from_list()) */
        init_hints(hints);
        hints->fabric_attr->prov_name = MPL_strdup(prov_use->fabric_attr->prov_name);
        hints->caps = prov_use->caps;
        hints->addr_format = prov_use->addr_format;

        fi_freeinfo(prov_list);
    } else {
        /* Make sure that the user-specified provider matches the configure-specified provider. */
        MPIR_ERR_CHKANDJUMP(provname != NULL &&
                            MPIDI_OFI_SET_NUMBER != MPIDI_OFI_get_set_number(provname),
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
        /* Initialize hints based on MPIDI_OFI_global.settings (config macros) */
        init_hints(hints);
        hints->fabric_attr->prov_name = provname ? MPL_strdup(provname) : NULL;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define DBG_TRY_PICK_PROVIDER(round) /* round is a str, eg "Round 1" */ \
    if (NULL == prov_use) { \
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, \
                        (MPL_DBG_FDEST, round ": find_provider returned NULL\n")); \
    } else { \
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, \
                        (MPL_DBG_FDEST, round ": find_provider returned %s\n", \
                        prov_use->fabric_attr->prov_name)); \
    }

static struct fi_info *pick_provider_from_list(const char *provname, struct fi_info *prov_list)
{
    struct fi_info *prov_use = NULL;
    /* We'll try to pick the best provider three times.
     * 1 - Check to see if any provider matches an existing capability set (e.g. sockets)
     * 2 - Check to see if any provider meets the default capability set
     * 3 - Check to see if any provider meets the minimal capability set
     */
    bool provname_is_set = (provname &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_DEFAULT) != 0 &&
                            strcmp(provname, MPIDI_OFI_SET_NAME_MINIMAL) != 0);
    if (NULL == prov_use && provname_is_set) {
        prov_use = pick_provider_by_name((char *) provname, prov_list);
        DBG_TRY_PICK_PROVIDER("[match name]");
    }

    bool provname_is_minimal = (provname && strcmp(provname, MPIDI_OFI_SET_NAME_MINIMAL) == 0);
    if (NULL == prov_use && !provname_is_minimal) {
        init_global_settings(MPIDI_OFI_SET_NAME_DEFAULT);
        prov_use = pick_provider_by_global_settings(prov_list);
        DBG_TRY_PICK_PROVIDER("[match default]");
    }

    if (NULL == prov_use) {
        init_global_settings(MPIDI_OFI_SET_NAME_MINIMAL);
        prov_use = pick_provider_by_global_settings(prov_list);
        DBG_TRY_PICK_PROVIDER("[match minimal]");
    }

    return prov_use;
}

static struct fi_info *pick_provider_by_name(const char *provname, struct fi_info *prov_list)
{
    struct fi_info *prov, *prov_use = NULL;

    prov = prov_list;
    while (NULL != prov) {
        /* Match provider name exactly */
        if (0 != strcmp(provname, prov->fabric_attr->prov_name)) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST, "Skipping provider: name mismatch"));
            prov = prov->next;
            continue;
        }

        init_global_settings(prov->fabric_attr->prov_name);

        if (!match_global_settings(prov)) {
            prov = prov->next;
            continue;
        }

        prov_use = prov;

        break;
    }

    return prov_use;
}

static struct fi_info *pick_provider_by_global_settings(struct fi_info *prov_list)
{
    struct fi_info *prov, *prov_use = NULL;

    prov = prov_list;
    while (NULL != prov) {
        if (!match_global_settings(prov)) {
            prov = prov->next;
            continue;
        } else {
            prov_use = prov;
            break;
        }
    }

    return prov_use;
}

#define CHECK_CAP(SETTING, cond_bad) \
    if (SETTING) { \
        if (cond_bad) { \
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, \
                            (MPL_DBG_FDEST, "provider failed " #SETTING)); \
            return false; \
        } \
    }

bool match_global_settings(struct fi_info * prov)
{
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, "Provider name: %s",
                                                     prov->fabric_attr->prov_name));

    if (MPIR_CVAR_OFI_SKIP_IPV6) {
        if (prov->addr_format == FI_SOCKADDR_IN6) {
            return false;
        }
    }
    CHECK_CAP(MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS,
              prov->domain_attr->max_ep_tx_ctx <= 1 ||
              (prov->caps & FI_NAMED_RX_CTX) != FI_NAMED_RX_CTX);

    /* From the fi_getinfo manpage: "FI_TAGGED implies the ability to send and receive
     * tagged messages." Therefore no need to specify FI_SEND|FI_RECV.  Moreover FI_SEND
     * and FI_RECV are mutually exclusive, so they should never be set both at the same
     * time. */
    /* This capability set also requires the ability to receive data in the completion
     * queue object (at least 32 bits). Previously, this was a separate capability set,
     * but as more and more providers supported this feature, the decision was made to
     * require it. */
    CHECK_CAP(MPIDI_OFI_ENABLE_TAGGED,
              !(prov->caps & FI_TAGGED) || prov->domain_attr->cq_data_size < 4);

    /* OFI provider doesn't expose FI_DIRECTED_RECV by default for performance consideration.
     * MPICH should request this flag to enable it. */
    if (MPIDI_OFI_ENABLE_TAGGED)
        prov->caps |= FI_DIRECTED_RECV;

    CHECK_CAP(MPIDI_OFI_ENABLE_AM,
              (prov->caps & (FI_MSG | FI_MULTI_RECV)) != (FI_MSG | FI_MULTI_RECV));

    CHECK_CAP(MPIDI_OFI_ENABLE_RMA, !(prov->caps & FI_RMA));
#ifdef FI_HMEM
    CHECK_CAP(MPIDI_OFI_ENABLE_HMEM, !(prov->caps & FI_HMEM));
#endif
    uint64_t msg_order = MPIDI_OFI_ATOMIC_ORDER_FLAGS;
    CHECK_CAP(MPIDI_OFI_ENABLE_ATOMICS,
              !(prov->caps & FI_ATOMICS) || (prov->tx_attr->msg_order & msg_order) != msg_order);

    CHECK_CAP(MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS,
              !(prov->domain_attr->control_progress & FI_PROGRESS_AUTO));

    CHECK_CAP(MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS,
              !(prov->domain_attr->data_progress & FI_PROGRESS_AUTO));

    int MPICH_REQUIRE_RDM = 1;  /* hack to use CHECK_CAP macro */
    CHECK_CAP(MPICH_REQUIRE_RDM, prov->ep_attr->type != FI_EP_RDM);

    return true;
}

#define UPDATE_SETTING_BY_CAP(cap, CVAR) \
    MPIDI_OFI_global.settings.cap = (CVAR != -1) ? CVAR : \
                                    prov_name ? prov_caps->cap : \
                                    CVAR

static void init_global_settings(const char *prov_name)
{
    int prov_idx = MPIDI_OFI_get_set_number(prov_name);
    MPIDI_OFI_capabilities_t *prov_caps = &MPIDI_OFI_caps_list[prov_idx];

    /* Seed the global settings values for cases where we are using runtime sets */
    UPDATE_SETTING_BY_CAP(enable_av_table, MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE);
    UPDATE_SETTING_BY_CAP(enable_scalable_endpoints, MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS);
    /* If the user specifies -1 (=don't care) and the provider supports it, then try to use STX
     * and fall back if necessary in the RMA init code */
    UPDATE_SETTING_BY_CAP(enable_shared_contexts, MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS);

    /* As of OFI version 1.5, FI_MR_SCALABLE and FI_MR_BASIC are deprecated. Internally, we now use
     * FI_MR_VIRT_ADDRESS and FI_MR_PROV_KEY so set them appropriately depending on the OFI version
     * being used. */
    if (get_ofi_version() < FI_VERSION(1, 5)) {
        /* If the OFI library is 1.5 or less, query whether or not to use FI_MR_SCALABLE and set
         * FI_MR_VIRT_ADDRESS, FI_MR_ALLOCATED, and FI_MR_PROV_KEY as the opposite values. */
        UPDATE_SETTING_BY_CAP(enable_mr_virt_address, MPIR_CVAR_CH4_OFI_ENABLE_MR_SCALABLE);
        MPIDI_OFI_global.settings.enable_mr_virt_address =
            MPIDI_OFI_global.settings.enable_mr_prov_key =
            MPIDI_OFI_global.settings.enable_mr_allocated =
            !MPIDI_OFI_global.settings.enable_mr_virt_address;
    } else {
        UPDATE_SETTING_BY_CAP(enable_mr_virt_address, MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS);
        UPDATE_SETTING_BY_CAP(enable_mr_allocated, MPIR_CVAR_CH4_OFI_ENABLE_MR_ALLOCATED);
        UPDATE_SETTING_BY_CAP(enable_mr_prov_key, MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY);
    }
    UPDATE_SETTING_BY_CAP(enable_tagged, MPIR_CVAR_CH4_OFI_ENABLE_TAGGED);
    UPDATE_SETTING_BY_CAP(enable_am, MPIR_CVAR_CH4_OFI_ENABLE_AM);
    UPDATE_SETTING_BY_CAP(enable_rma, MPIR_CVAR_CH4_OFI_ENABLE_RMA);
    /* try to enable atomics only when RMA is enabled */
    if (MPIDI_OFI_ENABLE_RMA) {
        UPDATE_SETTING_BY_CAP(enable_atomics, MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS);
    } else {
        MPIDI_OFI_global.settings.enable_atomics = 0;
    }
    UPDATE_SETTING_BY_CAP(fetch_atomic_iovecs, MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS);
    UPDATE_SETTING_BY_CAP(enable_data_auto_progress, MPIR_CVAR_CH4_OFI_ENABLE_DATA_AUTO_PROGRESS);
    UPDATE_SETTING_BY_CAP(enable_control_auto_progress,
                          MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS);
    UPDATE_SETTING_BY_CAP(enable_pt2pt_nopack, MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK);
    UPDATE_SETTING_BY_CAP(context_bits, MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS);
    UPDATE_SETTING_BY_CAP(source_bits, MPIR_CVAR_CH4_OFI_RANK_BITS);
    UPDATE_SETTING_BY_CAP(tag_bits, MPIR_CVAR_CH4_OFI_TAG_BITS);
    UPDATE_SETTING_BY_CAP(major_version, MPIR_CVAR_CH4_OFI_MAJOR_VERSION);
    UPDATE_SETTING_BY_CAP(minor_version, MPIR_CVAR_CH4_OFI_MINOR_VERSION);
    UPDATE_SETTING_BY_CAP(num_am_buffers, MPIR_CVAR_CH4_OFI_NUM_AM_BUFFERS);
    if (MPIDI_OFI_global.settings.num_am_buffers < 0) {
        MPIDI_OFI_global.settings.num_am_buffers = 0;
    }
    if (MPIDI_OFI_global.settings.num_am_buffers > MPIDI_OFI_MAX_NUM_AM_BUFFERS) {
        MPIDI_OFI_global.settings.num_am_buffers = MPIDI_OFI_MAX_NUM_AM_BUFFERS;
    }
}

#define UPDATE_SETTING_BY_INFO(cap, info_cond) \
    MPIDI_OFI_global.settings.cap = MPIDI_OFI_global.settings.cap && info_cond

static void update_global_settings(struct fi_info *prov_use, struct fi_info *hints)
{
    /* ------------------------------------------------------------------------ */
    /* Set global attributes attributes based on the provider choice            */
    /* ------------------------------------------------------------------------ */
    UPDATE_SETTING_BY_INFO(enable_av_table, prov_use->domain_attr->av_type == FI_AV_TABLE);
    UPDATE_SETTING_BY_INFO(enable_scalable_endpoints,
                           prov_use->domain_attr->max_ep_tx_ctx > 1 &&
                           (prov_use->caps & FI_NAMED_RX_CTX) == FI_NAMED_RX_CTX);
    /* As of OFI version 1.5, FI_MR_SCALABLE and FI_MR_BASIC are deprecated. Internally, we now use
     * FI_MR_VIRT_ADDRESS and FI_MR_PROV_KEY so set them appropriately depending on the OFI version
     * being used. */
    if (get_ofi_version() < FI_VERSION(1, 5)) {
        /* If the OFI library is 1.5 or less, query whether or not to use FI_MR_SCALABLE and set
         * FI_MR_VIRT_ADDRESS, FI_MR_ALLOCATED, and FI_MR_PROV_KEY as the opposite values. */
        UPDATE_SETTING_BY_INFO(enable_mr_virt_address,
                               prov_use->domain_attr->mr_mode != FI_MR_SCALABLE);
        UPDATE_SETTING_BY_INFO(enable_mr_allocated,
                               prov_use->domain_attr->mr_mode != FI_MR_SCALABLE);
        UPDATE_SETTING_BY_INFO(enable_mr_prov_key,
                               prov_use->domain_attr->mr_mode != FI_MR_SCALABLE);
    } else {
        UPDATE_SETTING_BY_INFO(enable_mr_virt_address,
                               prov_use->domain_attr->mr_mode & FI_MR_VIRT_ADDR);
        UPDATE_SETTING_BY_INFO(enable_mr_allocated,
                               prov_use->domain_attr->mr_mode & FI_MR_ALLOCATED);
        UPDATE_SETTING_BY_INFO(enable_mr_prov_key, prov_use->domain_attr->mr_mode & FI_MR_PROV_KEY);
    }
    UPDATE_SETTING_BY_INFO(enable_tagged,
                           (prov_use->caps & FI_TAGGED) &&
                           (prov_use->caps & FI_DIRECTED_RECV) &&
                           (prov_use->domain_attr->cq_data_size >= 4));
    UPDATE_SETTING_BY_INFO(enable_am,
                           (prov_use->caps & (FI_MSG | FI_MULTI_RECV | FI_READ)) ==
                           (FI_MSG | FI_MULTI_RECV | FI_READ));
    UPDATE_SETTING_BY_INFO(enable_rma, prov_use->caps & FI_RMA);
    UPDATE_SETTING_BY_INFO(enable_atomics, prov_use->caps & FI_ATOMICS);
#ifdef FI_HMEM
    UPDATE_SETTING_BY_INFO(enable_hmem, prov_use->caps & FI_HMEM);
#endif
    UPDATE_SETTING_BY_INFO(enable_data_auto_progress,
                           hints->domain_attr->data_progress & FI_PROGRESS_AUTO);
    UPDATE_SETTING_BY_INFO(enable_control_auto_progress,
                           hints->domain_attr->control_progress & FI_PROGRESS_AUTO);

    if (MPIDI_OFI_global.settings.enable_scalable_endpoints) {
        MPIDI_OFI_global.settings.max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_SCALABLE;
        MPIDI_OFI_global.settings.max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_SCALABLE;
    } else {
        MPIDI_OFI_global.settings.max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_REGULAR;
        MPIDI_OFI_global.settings.max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR;
    }
}

/* Initializes hint structure based MPIDI_OFI_global.settings (or config macros) */
static void init_hints(struct fi_info *hints)
{
    int ofi_version = get_ofi_version();
    MPIR_Assert(hints != NULL);

    /* ------------------------------------------------------------------------ */
    /* Hints to filter providers                                                */
    /* See man fi_getinfo for a list                                            */
    /* of all filters                                                           */
    /* mode:  Select capabilities that this netmod will support                 */
    /*        FI_CONTEXT(2):  This netmod will pass in context into communication */
    /*        to optimize storage locality between MPI requests and OFI opaque  */
    /*        data structures.                                                  */
    /*        FI_ASYNC_IOV:  MPICH will provide storage for iovecs on           */
    /*        communication calls, avoiding the OFI provider needing to require */
    /*        a copy.                                                           */
    /*        FI_LOCAL_MR unset:  Note that we do not set FI_LOCAL_MR,          */
    /*        which means this netmod does not support exchange of memory       */
    /*        regions on communication calls.                                   */
    /* caps:     Capabilities required from the provider.  The bits specified   */
    /*           with buffered receive, cancel, and remote complete implements  */
    /*           MPI semantics.                                                 */
    /*           Tagged: used to support tag matching, 2-sided                  */
    /*           RMA|Atomics:  supports MPI 1-sided                             */
    /*           MSG|MULTI_RECV:  Supports synchronization protocol for 1-sided */
    /*           FI_DIRECTED_RECV: Support not putting the source in the match  */
    /*                             bits                                         */
    /*           FI_NAMED_RX_CTX: Necessary to specify receiver-side SEP index  */
    /*                            when scalable endpoint (SEP) is enabled.      */
    /*           We expect to register all memory up front for use with this    */
    /*           endpoint, so the netmod requires dynamic memory regions        */
    /* ------------------------------------------------------------------------ */
    hints->mode = FI_CONTEXT | FI_ASYNC_IOV | FI_RX_CQ_DATA;    /* We can handle contexts  */

    if (ofi_version >= FI_VERSION(1, 5)) {
#ifdef FI_CONTEXT2
        hints->mode |= FI_CONTEXT2;
#endif
    }
    hints->caps = 0ULL;

    /* RMA interface is used in AM and in native modes,
     * it should be supported by OFI provider in any case */
    hints->caps |= FI_RMA;      /* RMA(read/write)         */
    hints->caps |= FI_WRITE;    /* We need to specify all of the extra
                                 * capabilities because we need to be
                                 * specific later when we create tx/rx
                                 * contexts. If we leave this off, the
                                 * context creation fails because it's not
                                 * a subset of this. */
    hints->caps |= FI_READ;
    hints->caps |= FI_REMOTE_WRITE;
    hints->caps |= FI_REMOTE_READ;

    if (MPIDI_OFI_ENABLE_ATOMICS) {
        hints->caps |= FI_ATOMICS;      /* Atomics capabilities    */
    }

    if (MPIDI_OFI_ENABLE_TAGGED) {
        hints->caps |= FI_TAGGED;       /* Tag matching interface  */
        hints->caps |= FI_DIRECTED_RECV;        /* Match source address    */
        hints->domain_attr->cq_data_size = 4;   /* Minimum size for completion data entry */
    }

    if (MPIDI_OFI_ENABLE_AM) {
        hints->caps |= FI_MSG;  /* Message Queue apis      */
        hints->caps |= FI_MULTI_RECV;   /* Shared receive buffer   */
    }

    /* With scalable endpoints, FI_NAMED_RX_CTX is needed to specify a destination receive context
     * index */
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS)
        hints->caps |= FI_NAMED_RX_CTX;

    /* ------------------------------------------------------------------------ */
    /* Set object options to be filtered by getinfo                             */
    /* domain_attr:  domain attribute requirements                              */
    /* op_flags:     persistent flag settings for an endpoint                   */
    /* endpoint type:  see FI_EP_RDM                                            */
    /* Filters applied (for this netmod, we need providers that can support):   */
    /* THREAD_DOMAIN:  Progress serialization is handled by netmod (locking)    */
    /* PROGRESS_AUTO:  request providers that make progress without requiring   */
    /*                 the ADI to dedicate a thread to advance the state        */
    /* FI_DELIVERY_COMPLETE:  RMA operations are visible in remote memory       */
    /* FI_COMPLETION:  Selective completions of RMA ops                         */
    /* FI_EP_RDM:  Reliable datagram                                            */
    /* ------------------------------------------------------------------------ */
    hints->addr_format = FI_FORMAT_UNSPEC;
    hints->domain_attr->threading = FI_THREAD_DOMAIN;
    if (MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS) {
        hints->domain_attr->data_progress = FI_PROGRESS_AUTO;
    } else {
        hints->domain_attr->data_progress = FI_PROGRESS_MANUAL;
    }
    if (MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS) {
        hints->domain_attr->control_progress = FI_PROGRESS_AUTO;
    } else {
        hints->domain_attr->control_progress = FI_PROGRESS_MANUAL;
    }
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->domain_attr->av_type = MPIDI_OFI_ENABLE_AV_TABLE ? FI_AV_TABLE : FI_AV_MAP;

    if (ofi_version >= FI_VERSION(1, 5)) {
        hints->domain_attr->mr_mode = 0;
#ifdef FI_RESTRICTED_COMP
        hints->domain_attr->mode = FI_RESTRICTED_COMP;
#endif
        /* avoid using FI_MR_SCALABLE and FI_MR_BASIC because they are only
         * for backward compatibility (pre OFI version 1.5), and they don't allow any other
         * mode bits to be added */
#ifdef FI_MR_VIRT_ADDR
        if (MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
            hints->domain_attr->mr_mode |= FI_MR_VIRT_ADDR;
        }
#endif

#ifdef FI_MR_ALLOCATED
        if (MPIDI_OFI_ENABLE_MR_ALLOCATED) {
            hints->domain_attr->mr_mode |= FI_MR_ALLOCATED;
        }
#endif

#ifdef FI_MR_PROV_KEY
        if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            hints->domain_attr->mr_mode |= FI_MR_PROV_KEY;
        }
#endif
    } else {
        if (MPIDI_OFI_ENABLE_MR_SCALABLE)
            hints->domain_attr->mr_mode = FI_MR_SCALABLE;
        else
            hints->domain_attr->mr_mode = FI_MR_BASIC;
    }
    hints->tx_attr->op_flags = FI_COMPLETION;
    hints->tx_attr->msg_order = FI_ORDER_SAS;
    /* direct RMA operations supported only with delivery complete mode,
     * else (AM mode) delivery complete is not required */
    if (MPIDI_OFI_ENABLE_RMA || MPIDI_OFI_ENABLE_ATOMICS) {
        hints->tx_attr->op_flags |= FI_DELIVERY_COMPLETE;
        /* Apply most restricted msg order in hints for RMA ATOMICS. */
        if (MPIDI_OFI_ENABLE_ATOMICS)
            hints->tx_attr->msg_order |= MPIDI_OFI_ATOMIC_ORDER_FLAGS;
    }
    hints->tx_attr->comp_order = FI_ORDER_NONE;
    hints->rx_attr->op_flags = FI_COMPLETION;
    hints->rx_attr->total_buffered_recv = 0;    /* FI_RM_ENABLED ensures buffering of unexpected messages */
    hints->ep_attr->type = FI_EP_RDM;
    hints->ep_attr->mem_tag_format = MPIDI_OFI_SOURCE_BITS ?
        /*     PROTOCOL         |  CONTEXT  |        SOURCE         |       TAG          */
        MPIDI_OFI_PROTOCOL_MASK | 0 | MPIDI_OFI_SOURCE_MASK | 0 /* With source bits */ :
        MPIDI_OFI_PROTOCOL_MASK | 0 | 0 | MPIDI_OFI_TAG_MASK /* No source bits */ ;
}

/* ---------------------------------------------------------- */
/* Debug Routines                                             */
/* ---------------------------------------------------------- */

static void dump_global_settings(void)
{
    fprintf(stdout, "==== Capability set configuration ====\n");
    fprintf(stdout, "libfabric provider: %s\n", MPIDI_OFI_global.prov_use->fabric_attr->prov_name);
    fprintf(stdout, "MPIDI_OFI_ENABLE_AV_TABLE: %d\n", MPIDI_OFI_ENABLE_AV_TABLE);
    fprintf(stdout, "MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS: %d\n",
            MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_SHARED_CONTEXTS: %d\n", MPIDI_OFI_ENABLE_SHARED_CONTEXTS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_SCALABLE: %d\n", MPIDI_OFI_ENABLE_MR_SCALABLE);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS: %d\n", MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_ALLOCATED: %d\n", MPIDI_OFI_ENABLE_MR_ALLOCATED);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_PROV_KEY: %d\n", MPIDI_OFI_ENABLE_MR_PROV_KEY);
    fprintf(stdout, "MPIDI_OFI_ENABLE_TAGGED: %d\n", MPIDI_OFI_ENABLE_TAGGED);
    fprintf(stdout, "MPIDI_OFI_ENABLE_AM: %d\n", MPIDI_OFI_ENABLE_AM);
    fprintf(stdout, "MPIDI_OFI_ENABLE_RMA: %d\n", MPIDI_OFI_ENABLE_RMA);
    fprintf(stdout, "MPIDI_OFI_ENABLE_ATOMICS: %d\n", MPIDI_OFI_ENABLE_ATOMICS);
    fprintf(stdout, "MPIDI_OFI_FETCH_ATOMIC_IOVECS: %d\n", MPIDI_OFI_FETCH_ATOMIC_IOVECS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS: %d\n",
            MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS: %d\n",
            MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_PT2PT_NOPACK: %d\n", MPIDI_OFI_ENABLE_PT2PT_NOPACK);
    fprintf(stdout, "MPIDI_OFI_ENABLE_HMEM: %d\n", MPIDI_OFI_ENABLE_HMEM);
    fprintf(stdout, "MPIDI_OFI_NUM_AM_BUFFERS: %d\n", MPIDI_OFI_NUM_AM_BUFFERS);
    fprintf(stdout, "MPIDI_OFI_CONTEXT_BITS: %d\n", MPIDI_OFI_CONTEXT_BITS);
    fprintf(stdout, "MPIDI_OFI_SOURCE_BITS: %d\n", MPIDI_OFI_SOURCE_BITS);
    fprintf(stdout, "MPIDI_OFI_TAG_BITS: %d\n", MPIDI_OFI_TAG_BITS);
    fprintf(stdout, "======================================\n");

    /* Discover the maximum number of ranks. If the source shift is not
     * defined, there are 32 bits in use due to the uint32_t used in
     * ofi_send.h */
    fprintf(stdout, "MAXIMUM SUPPORTED RANKS: %ld\n", (long int) 1 << MPIDI_OFI_MAX_RANK_BITS);

    /* Discover the tag_ub */
    fprintf(stdout, "MAXIMUM TAG: %lu\n", 1UL << MPIDI_OFI_TAG_BITS);
    fprintf(stdout, "======================================\n");
}

/* static address exchange routines */
static int addr_exchange_root_vni(MPIR_Comm * init_comm)
{
    int mpi_errno = MPI_SUCCESS;
    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;

    /* No pre-published address table, need do address exchange. */
    /* First, each get its own name */
    MPIDI_OFI_global.addrnamelen = FI_NAME_MAX;
    MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_OFI_global.ctx[0].ep, MPIDI_OFI_global.addrname,
                              &MPIDI_OFI_global.addrnamelen), getname);
    MPIR_Assert(MPIDI_OFI_global.addrnamelen <= FI_NAME_MAX);

    /* Second, exchange names using PMI */
    /* If MPIR_CVAR_CH4_ROOTS_ONLY_PMI is true, we only collect a table of node-roots.
     * Otherwise, we collect a table of everyone. */
    void *table = NULL;
    int ret_bc_len;
    mpi_errno = MPIDU_bc_table_create(rank, size, MPIDI_global.node_map[0],
                                      &MPIDI_OFI_global.addrname, MPIDI_OFI_global.addrnamelen,
                                      TRUE, MPIR_CVAR_CH4_ROOTS_ONLY_PMI, &table, &ret_bc_len);
    MPIR_ERR_CHECK(mpi_errno);
    /* MPIR_Assert(ret_bc_len = MPIDI_OFI_global.addrnamelen); */

    /* Third, each fi_av_insert those addresses */
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        /* if "ROOTS_ONLY", we do a two stage bootstrapping ... */
        int num_nodes = MPIR_Process.num_nodes;
        int *node_roots = MPIR_Process.node_root_map;
        int *rank_map, recv_bc_len;

        /* First, insert address of node-roots, init_comm become useful */
        fi_addr_t *mapped_table;
        mapped_table = (fi_addr_t *) MPL_malloc(num_nodes * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
        MPIDI_OFI_CALL(fi_av_insert
                       (MPIDI_OFI_global.ctx[0].av, table, num_nodes, mapped_table, 0ULL, NULL),
                       avmap);

        for (int i = 0; i < num_nodes; i++) {
            MPIR_Assert(mapped_table[i] != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV(&MPIDIU_get_av(0, node_roots[i])).dest[0][0] = mapped_table[i];
        }
        MPL_free(mapped_table);
        /* Then, allgather all address names using init_comm */
        MPIDU_bc_allgather(init_comm, MPIDI_OFI_global.addrname, MPIDI_OFI_global.addrnamelen,
                           TRUE, &table, &rank_map, &recv_bc_len);

        /* Insert the rest of the addresses */
        for (int i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {
                fi_addr_t addr;
                char *addrname = (char *) table + recv_bc_len * rank_map[i];
                MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[0].av,
                                            addrname, 1, &addr, 0ULL, NULL), avmap);
                MPIDI_OFI_AV(&MPIDIU_get_av(0, rank)).dest[0][0] = addr;
            }
        }
        MPIDU_bc_table_destroy();
    } else {
        /* not "ROOTS_ONLY", we already have everyone's address name, insert all of them */
        fi_addr_t *mapped_table;
        mapped_table = (fi_addr_t *) MPL_malloc(size * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
        MPIDI_OFI_CALL(fi_av_insert
                       (MPIDI_OFI_global.ctx[0].av, table, size, mapped_table, 0ULL, NULL), avmap);

        for (int i = 0; i < size; i++) {
            MPIR_Assert(mapped_table[i] != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest[0][0] = mapped_table[i];
        }
        MPL_free(mapped_table);
        MPIDU_bc_table_destroy();
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int addr_exchange_all_vnis(void)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    int num_vnis = MPIDI_OFI_global.num_vnis;

    /* get addr name length */
    size_t name_len = 0;
    int ret = fi_getname((fid_t) MPIDI_OFI_global.ctx[0].ep, NULL, &name_len);
    MPIR_Assert(ret == -FI_ETOOSMALL);
    MPIR_Assert(name_len > 0);

    int my_len = num_vnis * name_len;
    char *all_names = MPL_malloc(size * my_len, MPL_MEM_ADDRESS);
    MPIR_Assert(all_names);

    char *my_names = all_names + rank * my_len;

    /* put in my addrnames */
    for (int i = 0; i < num_vnis; i++) {
        size_t actual_name_len = name_len;
        char *vni_addrname = my_names + i * name_len;
        MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_OFI_global.ctx[i].ep, vni_addrname,
                                  &actual_name_len), getname);
        MPIR_Assert(actual_name_len == name_len);
    }
    /* Allgather */
    MPIR_Comm *comm = MPIR_Process.comm_world;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    mpi_errno = MPIR_Allgather_allcomm_auto(MPI_IN_PLACE, 0, MPI_BYTE,
                                            all_names, my_len, MPI_BYTE, comm, &errflag);
    /* insert the addresses */
    fi_addr_t *mapped_table;
    mapped_table = (fi_addr_t *) MPL_malloc(size * num_vnis * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
    for (int vni_local = 0; vni_local < num_vnis; vni_local++) {
        MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[vni_local].av, all_names, size * num_vnis,
                                    mapped_table, 0ULL, NULL), avmap);
        for (int r = 0; r < size; r++) {
            MPIDI_OFI_addr_t *av = &MPIDI_OFI_AV(&MPIDIU_get_av(0, r));
            for (int vni_remote = 0; vni_remote < num_vnis; vni_remote++) {
                if (vni_local == 0 && vni_remote == 0) {
                    /* don't overwrite existing addr, or bad things will happen */
                    continue;
                }
                int idx = r * num_vnis + vni_remote;
                MPIR_Assert(mapped_table[idx] != FI_ADDR_NOTAVAIL);
                av->dest[vni_local][vni_remote] = mapped_table[idx];
            }
        }
    }
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
