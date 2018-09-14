/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_INIT_H_INCLUDED
#define OFI_INIT_H_INCLUDED

#include "ofi_impl.h"
#include "mpir_cvars.h"
#include "mpidu_bc.h"

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
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Prints out the configuration of each capability selected via the capability sets interface.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_DATA
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Enable immediate data fields in OFI to transmit source rank outside of the match bits

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, the OFI addressing information will be stored with an FI_AV_TABLE.
        If false, an FI_AV_MAP will be used.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, use OFI scalable endpoints.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
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
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, MR_SCALABLE for OFI memory regions.
        If false, MR_BASIC for OFI memory regions.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_TAGGED
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, use tagged message transmission functions in OFI.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_AM
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI active message support.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_RMA
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI RMA support.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable OFI Atomics support.

    - name        : MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
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
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable MPI data auto progress.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable MPI control auto progress.

    - name        : MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, enable iovec for pt2pt.

    - name        : MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS
      category    : CH4_OFI
      type        : int
      default     : -1
      class       : device
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
      class       : device
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
      class       : device
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
      class       : device
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
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the major version of the OFI library. The default is the
        minor version of the OFI library used with MPICH. If using this CVAR,
        it is recommended that the user also specifies a specific OFI provider.

    - name        : MPIR_CVAR_CH4_OFI_MAX_VNIS
      category    : CH4_OFI
      type        : int
      default     : 1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of CH4 VNIs
        that OFI netmod exposes.

    - name        : MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX
      category    : CH4_OFI
      type        : int
      default     : 0
      class       : device
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
      class       : device
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
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of buffers for receiving active messages.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static inline void MPIDI_OFI_dump_providers(struct fi_info *prov);
static inline int MPIDI_OFI_create_endpoint(struct fi_info *prov_use,
                                            struct fid_domain *domain,
                                            struct fid_cq *p2p_cq,
                                            struct fid_cntr *rma_ctr,
                                            struct fid_av *av, struct fid_ep **ep, int index);
static inline int MPIDI_OFI_application_hints(int rank);
static inline int MPIDI_OFI_init_global_settings(const char *prov_name);
static inline int MPIDI_OFI_init_hints(struct fi_info *hints);

static inline int MPIDI_OFI_set_eagain(MPIR_Comm * comm_ptr, MPIR_Info * info, void *state)
{
    if (!strncmp(info->value, "true", strlen("true")))
        MPIDI_OFI_COMM(comm_ptr).eagain = TRUE;
    if (!strncmp(info->value, "false", strlen("false")))
        MPIDI_OFI_COMM(comm_ptr).eagain = FALSE;

    return MPI_SUCCESS;
}

static inline int MPIDI_OFI_conn_manager_init()
{
    int mpi_errno = MPI_SUCCESS, i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INIT);

    MPIDI_Global.conn_mgr.mmapped_size = 8 * 4 * 1024;
    MPIDI_Global.conn_mgr.max_n_conn = 1;
    MPIDI_Global.conn_mgr.next_conn_id = 0;
    MPIDI_Global.conn_mgr.n_conn = 0;

    MPIDI_Global.conn_mgr.conn_list =
        (MPIDI_OFI_conn_t *) MPL_mmap(NULL, MPIDI_Global.conn_mgr.mmapped_size,
                                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0,
                                      MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_Global.conn_mgr.conn_list == MAP_FAILED, mpi_errno, MPI_ERR_NO_MEM,
                        goto fn_fail, "**nomem");

    MPIDI_Global.conn_mgr.free_conn_id =
        (int *) MPL_malloc(MPIDI_Global.conn_mgr.max_n_conn * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_Global.conn_mgr.free_conn_id == NULL, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    for (i = 0; i < MPIDI_Global.conn_mgr.max_n_conn; ++i) {
        MPIDI_Global.conn_mgr.free_conn_id[i] = i + 1;
    }
    MPIDI_Global.conn_mgr.free_conn_id[MPIDI_Global.conn_mgr.max_n_conn - 1] = -1;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_conn_manager_destroy()
{
    int mpi_errno = MPI_SUCCESS, i, j;
    MPIDI_OFI_dynamic_process_request_t *req;
    fi_addr_t *conn;
    int max_n_conn = MPIDI_Global.conn_mgr.max_n_conn;
    int *close_msg;
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    MPIR_Context_id_t context_id = 0xF000;
    MPIR_CHKLMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONN_MANAGER_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONN_MANAGER_DESTROY);

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, MPI_ANY_SOURCE, 1);
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
            switch (MPIDI_Global.conn_mgr.conn_list[i].state) {
                case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
                    MPIDI_OFI_dynproc_send_disconnect(i);
                    break;
                case MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT:
                case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
                    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                    (MPL_DBG_FDEST, "Wait for close of conn_id=%d", i));
                    conn[j] = MPIDI_Global.conn_mgr.conn_list[i].dest;
                    req[j].done = 0;
                    req[j].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
                    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_Global.ctx[0].rx,
                                                  &close_msg[j],
                                                  sizeof(int),
                                                  NULL,
                                                  conn[j],
                                                  match_bits,
                                                  mask_bits, &req[j].context),
                                         trecv, MPIDI_OFI_CALL_LOCK, FALSE);
                    j++;
                    break;
                default:
                    break;
            }
        }

        for (i = 0; i < j; ++i) {
            MPIDI_OFI_PROGRESS_WHILE(!req[i].done);
            MPIDI_Global.conn_mgr.conn_list[i].state = MPIDI_OFI_DYNPROC_DISCONNECTED;
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST, "conn_id=%d closed", i));
        }

        MPIR_CHKLMEM_FREEALL();
    }

    MPL_munmap((void *) MPIDI_Global.conn_mgr.conn_list, MPIDI_Global.conn_mgr.mmapped_size,
               MPL_MEM_ADDRESS);
    MPL_free(MPIDI_Global.conn_mgr.free_conn_id);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONN_MANAGER_DESTROY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_init_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_init_hook(int rank,
                                         int size,
                                         int appnum,
                                         int *tag_bits,
                                         MPIR_Comm * comm_world,
                                         MPIR_Comm * comm_self, int spawned, int *n_vnis_provided)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno, i, fi_version;
    int thr_err = 0;
    void *table = NULL, *provname = NULL;
    struct fi_info *hints, *prov = NULL, *prov_use, *prov_first;
    struct fi_cq_attr cq_attr;
    struct fi_cntr_attr cntr_attr;
    fi_addr_t *mapped_table;
    struct fi_av_attr av_attr;
    size_t optlen;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_INIT);

    if (!MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        MPIDI_OFI_init_global_settings(MPIR_CVAR_OFI_USE_PROVIDER);
    } else {
        /* when runtime checks are enabled, and no provider name is set, then
         * try to select the fastest provider which fit minimal requirements.
         * to do this use "unknown" fake provider name to force initialization
         * of global settings by default minimal values. We are assuming
         * that libfabric returns available provider list sorted by performance
         * (faster - first) */
        MPIDI_OFI_init_global_settings(MPIR_CVAR_OFI_USE_PROVIDER ? MPIR_CVAR_OFI_USE_PROVIDER :
                                       "unknown");
    }

    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_chunk_request, context));
    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_huge_recv_t, context));
    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_am_repost_request_t, context));
    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_ssendack_request_t, context));
    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_dynamic_process_request_t, context));
    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.netmod) ==
                            offsetof(MPIDI_OFI_win_request_t, context));
    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.am.netmod_am.ofi.context) ==
                            offsetof(struct MPIR_Request, dev.ch4.netmod.ofi.context));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devreq_t) >= sizeof(MPIDI_OFI_request_t));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIR_Request) >= sizeof(MPIDI_OFI_win_request_t));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIR_Context_id_t) * 8 >= MPIDI_OFI_AM_CONTEXT_ID_BITS);

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_UTIL_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_FI_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &thr_err);

    /* ------------------------------------------------------------------------ */
    /* fi_allocinfo: allocate and zero an fi_info structure and all related     */
    /* substructures                                                            */
    /* ------------------------------------------------------------------------ */
    hints = fi_allocinfo();
    MPIR_Assert(hints != NULL);

    /* ------------------------------------------------------------------------ */
    /* FI_VERSION provides binary backward and forward compatibility support    */
    /* Specify the version of OFI is coded to, the provider will select struct  */
    /* layouts that are compatible with this version.                           */
    /* ------------------------------------------------------------------------ */
    if (MPIDI_OFI_MAJOR_VERSION != -1 && MPIDI_OFI_MINOR_VERSION != -1)
        fi_version = FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION);
    else
        fi_version = FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION);

    MPIDI_OFI_init_hints(hints);

    /* ------------------------------------------------------------------------ */
    /* fi_getinfo:  returns information about fabric  services for reaching a   */
    /* remote node or service.  this does not necessarily allocate resources.   */
    /* Pass NULL for name/service because we want a list of providers supported */
    /* ------------------------------------------------------------------------ */
    provname = MPIR_CVAR_OFI_USE_PROVIDER ? (char *) MPL_strdup(MPIR_CVAR_OFI_USE_PROVIDER) : NULL;
    hints->fabric_attr->prov_name = provname;

    /* If the user picked a particular provider, ignore the checks */
    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        /* Ensure that we aren't trying to shove too many bits into the match_bits.
         * Currently, this needs to fit into a uint64_t and we take 4 bits for protocol. */
        MPIR_Assert(MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS <= 60);

        MPIDI_OFI_CALL(fi_getinfo(fi_version, NULL, NULL, 0ULL, NULL, &prov), addrinfo);
        prov_first = prov;
        while (NULL != prov) {
            prov_use = prov;

            /* If we picked the provider already, make sure we grab the right provider */
            if ((NULL != provname) && (0 != strcmp(provname, prov_use->fabric_attr->prov_name))) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Skipping provider because not selected one"));
                prov = prov_use->next;
                continue;
            }

            /* set global settings according to evaluated provider */
            if (!MPIR_CVAR_OFI_USE_PROVIDER)
                MPIDI_OFI_init_global_settings(prov_use->fabric_attr->prov_name);

            /* Check that this provider meets the minimum requirements for the user */
            if (MPIDI_OFI_ENABLE_DATA && (!(prov_use->caps & FI_DIRECTED_RECV) ||
                                          prov_use->domain_attr->cq_data_size < 4)) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support immediate data"));
                prov = prov_use->next;
                continue;
            } else if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS &&
                       (prov_use->domain_attr->max_ep_tx_ctx <= 1)) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support scalable endpoints"));
                prov = prov_use->next;
                continue;
            } else if (MPIDI_OFI_ENABLE_TAGGED && !(prov_use->caps & FI_TAGGED)) {
                /* From the fi_getinfo manpage: "FI_TAGGED implies the
                 * ability to send and receive tagged messages."
                 * Therefore no need to specify FI_SEND|FI_RECV.
                 * Moreover FI_SEND and FI_RECV are mutually
                 * exclusive, so they should never be set both at the
                 * same time. */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support tagged interfaces"));
                prov = prov_use->next;
                continue;
            } else if (MPIDI_OFI_ENABLE_AM &&
                       ((prov_use->caps & (FI_MSG | FI_MULTI_RECV)) != (FI_MSG | FI_MULTI_RECV))) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support active messages"));
                prov = prov_use->next;
                continue;
            } else if (MPIDI_OFI_ENABLE_RMA && !(prov_use->caps & FI_RMA)) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support RMA"));
                prov = prov_use->next;
                continue;
            } else if (MPIDI_OFI_ENABLE_ATOMICS && !(prov_use->caps & FI_ATOMICS)) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support atomics"));
                prov = prov_use->next;
                continue;
            } else if (MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS &&
                       !(hints->domain_attr->control_progress & FI_PROGRESS_AUTO)) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support auto control progress"));
                prov = prov_use->next;
                continue;
            } else if (MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS &&
                       !(hints->domain_attr->data_progress & FI_PROGRESS_AUTO)) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support auto data progress"));
                prov = prov_use->next;
                continue;

                /* Check that the provider has all of the requirements of MPICH */
            } else if (prov_use->ep_attr->type != FI_EP_RDM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "Provider doesn't support RDM"));
                prov = prov_use->next;
                continue;
            }

            /* If the provider passed the above tests, then run the selection
             * logic again and make sure to pick this provider again with the
             * hints included this time. */
            hints->caps = prov_use->caps;
            if (!MPIR_CVAR_OFI_USE_PROVIDER) {
                /* as soon as we updates globals for the provider, let's update
                 * hints that corresponds to the current globals */
                MPIDI_OFI_init_hints(hints);
                /* set provider name to hints to make more accurate selection */
                provname = MPL_strdup(prov_use->fabric_attr->prov_name);
                hints->fabric_attr->prov_name = provname;
            }
            break;
        }

        if (prov_first && !prov && !MPIR_CVAR_OFI_USE_PROVIDER) {
            /* could not find suitable provider. ok, let's try fallback mode */
            MPIDI_OFI_init_global_settings(prov_first->fabric_attr->prov_name);
            MPIDI_OFI_init_hints(hints);
            MPIDI_OFI_CALL(fi_getinfo(fi_version, NULL, NULL, 0ULL, hints, &prov), addrinfo);
            MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_addrinfo");
            prov_use = prov;

            if (prov_use) {
                /* If the provider passed the above tests, then run the selection
                 * logic again and make sure to pick this provider again with the
                 * hints included this time. */
                hints->caps = prov_use->caps;
                if (!MPIR_CVAR_OFI_USE_PROVIDER) {
                    /* set provider name to hints to make more accurate selection */
                    provname = MPL_strdup(prov_use->fabric_attr->prov_name);
                    hints->fabric_attr->prov_name = provname;
                }
            }
            if (prov)
                fi_freeinfo(prov);
        }

        /* If we did not find a provider, return an error */
        MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_addrinfo");

        fi_freeinfo(prov_first);
    } else {
        /* If runtime checks are disabled, make sure that the user-specified
         * provider matches the configure-specified provider. */
        MPIR_ERR_CHKANDJUMP(provname != NULL &&
                            MPIDI_OFI_SET_NUMBER != MPIDI_OFI_get_set_number(provname),
                            mpi_errno, MPI_ERR_OTHER, "**ofi_provider_mismatch");
    }

    MPIDI_OFI_CALL(fi_getinfo(fi_version, NULL, NULL, 0ULL, hints, &prov), addrinfo);
    MPIR_ERR_CHKANDJUMP(prov == NULL, mpi_errno, MPI_ERR_OTHER, "**ofid_addrinfo");
    /* When a specific provider is specified at configure time,
     * make sure it is the one selected by fi_getinfo */
    MPIR_ERR_CHKANDJUMP(!MPIDI_OFI_ENABLE_RUNTIME_CHECKS &&
                        MPIDI_OFI_SET_NUMBER !=
                        MPIDI_OFI_get_set_number(prov->fabric_attr->prov_name), mpi_errno,
                        MPI_ERR_OTHER, "**ofi_provider_mismatch");
    prov_use = prov;
    if (MPIR_CVAR_OFI_DUMP_PROVIDERS)
        MPIDI_OFI_dump_providers(prov);

    *tag_bits = MPIDI_OFI_TAG_BITS;

    if (MPIDI_OFI_ENABLE_RUNTIME_CHECKS) {
        /* ------------------------------------------------------------------------ */
        /* Set global attributes attributes based on the provider choice            */
        /* ------------------------------------------------------------------------ */
        MPIDI_Global.settings.enable_data = MPIDI_Global.settings.enable_data &&
            (prov_use->caps & FI_DIRECTED_RECV) && prov_use->domain_attr->cq_data_size >= 4;
        MPIDI_Global.settings.enable_av_table = MPIDI_Global.settings.enable_av_table &&
            prov_use->domain_attr->av_type == FI_AV_TABLE;
        MPIDI_Global.settings.enable_scalable_endpoints =
            MPIDI_Global.settings.enable_scalable_endpoints &&
            prov_use->domain_attr->max_ep_tx_ctx > 1;
        MPIDI_Global.settings.enable_mr_scalable =
            MPIDI_Global.settings.enable_mr_scalable &&
            prov_use->domain_attr->mr_mode == FI_MR_SCALABLE;
        MPIDI_Global.settings.enable_tagged = MPIDI_Global.settings.enable_tagged &&
            prov_use->caps & FI_TAGGED;
        MPIDI_Global.settings.enable_am = MPIDI_Global.settings.enable_am &&
            (prov_use->caps & (FI_MSG | FI_MULTI_RECV | FI_READ)) ==
            (FI_MSG | FI_MULTI_RECV | FI_READ);
        MPIDI_Global.settings.enable_rma = MPIDI_Global.settings.enable_rma &&
            prov_use->caps & FI_RMA;
        MPIDI_Global.settings.enable_atomics = MPIDI_Global.settings.enable_atomics &&
            prov_use->caps & FI_ATOMICS;
        MPIDI_Global.settings.enable_data_auto_progress =
            MPIDI_Global.settings.enable_data_auto_progress &&
            hints->domain_attr->data_progress & FI_PROGRESS_AUTO;
        MPIDI_Global.settings.enable_control_auto_progress =
            MPIDI_Global.settings.enable_control_auto_progress &&
            hints->domain_attr->control_progress & FI_PROGRESS_AUTO;

        if (MPIDI_Global.settings.enable_scalable_endpoints) {
            MPIDI_Global.settings.max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_SCALABLE;
            MPIDI_Global.settings.max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_SCALABLE;
        } else {
            MPIDI_Global.settings.max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_REGULAR;
            MPIDI_Global.settings.max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR;
        }
    }

    /* Print some debugging output to give the user some hints */
    mpi_errno = MPIDI_OFI_application_hints(rank);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDI_Global.prov_use = fi_dupinfo(prov_use);
    MPIR_Assert(MPIDI_Global.prov_use);

    MPIDI_Global.max_buffered_send = prov_use->tx_attr->inject_size;
    MPIDI_Global.max_buffered_write = prov_use->tx_attr->inject_size;
    MPIDI_Global.max_send = prov_use->ep_attr->max_msg_size;
    MPIDI_Global.max_write = prov_use->ep_attr->max_msg_size;
    MPIDI_Global.max_order_raw = prov_use->ep_attr->max_order_raw_size;
    MPIDI_Global.max_order_war = prov_use->ep_attr->max_order_war_size;
    MPIDI_Global.max_order_waw = prov_use->ep_attr->max_order_waw_size;
    MPIDI_Global.tx_iov_limit = MIN(prov_use->tx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_Global.rx_iov_limit = MIN(prov_use->rx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_Global.rma_iov_limit = MIN(prov_use->tx_attr->rma_iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_Global.max_mr_key_size = prov_use->domain_attr->mr_key_size;

    if (MPIDI_Global.max_mr_key_size >= 8) {
        MPIDI_Global.max_rma_key_bits = MPIDI_OFI_MAX_KEY_BITS_64;
        MPIDI_Global.max_huge_rmas = MPIDI_OFI_MAX_HUGE_RMAS_64;
        MPIDI_Global.context_shift = MPIDI_OFI_CONTEXT_SHIFT_64;
        MPIDI_Global.rma_key_type_bits = MPIDI_OFI_MAX_KEY_TYPE_BITS_64;
    } else if (MPIDI_Global.max_mr_key_size >= 4) {
        MPIDI_Global.max_rma_key_bits = MPIDI_OFI_MAX_KEY_BITS_32;
        MPIDI_Global.max_huge_rmas = MPIDI_OFI_MAX_HUGE_RMAS_32;
        MPIDI_Global.context_shift = MPIDI_OFI_CONTEXT_SHIFT_32;
        MPIDI_Global.rma_key_type_bits = MPIDI_OFI_MAX_KEY_TYPE_BITS_32;
    } else if (MPIDI_Global.max_mr_key_size >= 2) {
        MPIDI_Global.max_rma_key_bits = MPIDI_OFI_MAX_KEY_BITS_16;
        MPIDI_Global.max_huge_rmas = MPIDI_OFI_MAX_HUGE_RMAS_16;
        MPIDI_Global.context_shift = MPIDI_OFI_CONTEXT_SHIFT_16;
        MPIDI_Global.rma_key_type_bits = MPIDI_OFI_MAX_KEY_TYPE_BITS_16;
    } else {
        MPIR_ERR_SETFATALANDJUMP4(mpi_errno,
                                  MPI_ERR_OTHER,
                                  "**ofid_rma_init",
                                  "**ofid_rma_init %s %d %s %s",
                                  __SHORT_FILE__, __LINE__, FCNAME, "Key space too small");
    }

    /* ------------------------------------------------------------------------ */
    /* Open fabric                                                              */
    /* The getinfo struct returns a fabric attribute struct that can be used to */
    /* instantiate the virtual or physical network.  This opens a "fabric       */
    /* provider".   We choose the first available fabric, but getinfo           */
    /* returns a list.                                                          */
    /* ------------------------------------------------------------------------ */
    MPIDI_OFI_CALL(fi_fabric(prov_use->fabric_attr, &MPIDI_Global.fabric, NULL), fabric);

    /* ------------------------------------------------------------------------ */
    /* Create the access domain, which is the physical or virtual network or    */
    /* hardware port/collection of ports.  Returns a domain object that can be  */
    /* used to create endpoints.                                                */
    /* ------------------------------------------------------------------------ */
    MPIDI_OFI_CALL(fi_domain(MPIDI_Global.fabric, prov_use, &MPIDI_Global.domain, NULL),
                   opendomain);

    /* ------------------------------------------------------------------------ */
    /* Create the objects that will be bound to the endpoint.                   */
    /* The objects include:                                                     */
    /*     * dynamic memory-spanning memory region                              */
    /*     * completion queues for events                                       */
    /*     * counters for rma operations                                        */
    /*     * address vector of other endpoint addresses                         */
    /* ------------------------------------------------------------------------ */

    /* ------------------------------------------------------------------------ */
    /* Construct:  Completion Queues                                            */
    /* ------------------------------------------------------------------------ */
    memset(&cq_attr, 0, sizeof(cq_attr));
    cq_attr.format = FI_CQ_FORMAT_TAGGED;
    MPIDI_OFI_CALL(fi_cq_open(MPIDI_Global.domain,      /* In:  Domain Object                */
                              &cq_attr, /* In:  Configuration object         */
                              &MPIDI_Global.p2p_cq,     /* Out: CQ Object                    */
                              NULL), opencq);   /* In:  Context for cq events        */

    /* ------------------------------------------------------------------------ */
    /* Construct:  Counters                                                     */
    /* ------------------------------------------------------------------------ */
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    cntr_attr.events = FI_CNTR_EVENTS_COMP;
    cntr_attr.wait_obj = FI_WAIT_UNSPEC;
    MPIDI_OFI_CALL(fi_cntr_open(MPIDI_Global.domain,    /* In:  Domain Object        */
                                &cntr_attr,     /* In:  Configuration object */
                                &MPIDI_Global.rma_cmpl_cntr,    /* Out: Counter Object       */
                                NULL), openct); /* Context: counter events   */

    /* ------------------------------------------------------------------------ */
    /* Construct:  Address Vector                                               */
    /* ------------------------------------------------------------------------ */

    memset(&av_attr, 0, sizeof(av_attr));


    if (MPIDI_OFI_ENABLE_AV_TABLE) {
        av_attr.type = FI_AV_TABLE;
    } else {
        av_attr.type = FI_AV_MAP;
    }

    av_attr.rx_ctx_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS;

    /* ------------------------------------------------------------------------ */
    /* Attempt to open a shared address vector read-only. The open will fail if */
    /* the address vector does not exist, otherwise the location of the mapped  */
    /* fi_addr_t array will be returned in the 'map_addr' field of the address  */
    /* vector attribute structure.                                              */
    /* ------------------------------------------------------------------------ */
    char av_name[128];
    MPL_snprintf(av_name, sizeof(av_name), "FI_NAMED_AV_%d\n", appnum);
    av_attr.name = av_name;
    av_attr.flags = FI_READ;
    av_attr.map_addr = 0;

    unsigned do_av_insert = 1;
    if (0 == fi_av_open(MPIDI_Global.domain,    /* In:  Domain Object         */
                        &av_attr,       /* In:  Configuration object  */
                        &MPIDI_Global.av,       /* Out: AV Object             */
                        NULL)) {        /* Context: AV events         */
        do_av_insert = 0;

        /* TODO - the copy from the pre-existing av map into the 'MPIDI_OFI_AV' */
        /* is wasteful and should be changed so that the 'MPIDI_OFI_AV' object  */
        /* directly references the mapped fi_addr_t array instead               */
        mapped_table = (fi_addr_t *) av_attr.map_addr;
        for (i = 0; i < size; i++) {
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest = mapped_table[i];
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#else
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#endif
#endif
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " grank mapped to: rank=%d, av=%p, dest=%" PRIu64,
                             i, (void *) &MPIDIU_get_av(0, i), mapped_table[i]));
        }
        mapped_table = NULL;
    } else {
        av_attr.name = NULL;
        av_attr.flags = 0;
        MPIDI_OFI_CALL(fi_av_open(MPIDI_Global.domain,  /* In:  Domain Object    */
                                  &av_attr,     /* In:  Configuration object */
                                  &MPIDI_Global.av,     /* Out: AV Object            */
                                  NULL), avopen);       /* Context: AV events        */
    }

    /* ------------------------------------------------------------------------ */
    /* Construct:  Shared TX Context for RMA                                    */
    /* ------------------------------------------------------------------------ */
    if (MPIDI_OFI_ENABLE_SHARED_CONTEXTS) {
        int ret;
        struct fi_tx_attr tx_attr;
        memset(&tx_attr, 0, sizeof(tx_attr));
        /* A shared transmit context’s attributes must be a union of all associated
         * endpoints' transmit capabilities. */
        tx_attr.caps = FI_RMA | FI_WRITE | FI_READ | FI_ATOMIC;
        tx_attr.msg_order = FI_ORDER_RAR | FI_ORDER_RAW | FI_ORDER_WAR | FI_ORDER_WAW;
        tx_attr.op_flags = FI_DELIVERY_COMPLETE | FI_COMPLETION;
        MPIDI_OFI_CALL_RETURN(fi_stx_context(MPIDI_Global.domain,
                                             &tx_attr,
                                             &MPIDI_Global.rma_stx_ctx, NULL /* context */), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to create shared TX context for RMA, "
                        "falling back to global EP/counter scheme");
            MPIDI_Global.rma_stx_ctx = NULL;
        }
    }

    /* ------------------------------------------------------------------------ */
    /* Create a transport level communication endpoint.  To use the endpoint,   */
    /* it must be bound to completion counters or event queues and enabled,     */
    /* and the resources consumed by it, such as address vectors, counters,     */
    /* completion queues, etc.                                                  */
    /* ------------------------------------------------------------------------ */

    MPIDI_Global.max_ch4_vnis = 1;
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        int max_by_prov = MPL_MIN(prov_use->domain_attr->tx_ctx_cnt,
                                  prov_use->domain_attr->rx_ctx_cnt);
        if (MPIR_CVAR_CH4_OFI_MAX_VNIS > 0)
            MPIDI_Global.max_ch4_vnis = MPL_MIN(MPIR_CVAR_CH4_OFI_MAX_VNIS, max_by_prov);
        if (MPIDI_Global.max_ch4_vnis < 1) {
            MPIR_ERR_SETFATALANDJUMP4(mpi_errno,
                                      MPI_ERR_OTHER,
                                      "**ofid_ep",
                                      "**ofid_ep %s %d %s %s",
                                      __SHORT_FILE__, __LINE__, FCNAME,
                                      "Not enough scalable endpoints");
        }
        /* Specify the number of TX/RX contexts we want */
        prov_use->ep_attr->tx_ctx_cnt = MPIDI_Global.max_ch4_vnis;
        prov_use->ep_attr->rx_ctx_cnt = MPIDI_Global.max_ch4_vnis;
    }

    for (i = 0; i < MPIDI_Global.max_ch4_vnis; i++) {
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_create_endpoint(prov_use,
                                                         MPIDI_Global.domain,
                                                         MPIDI_Global.p2p_cq,
                                                         MPIDI_Global.rma_cmpl_cntr,
                                                         MPIDI_Global.av, &MPIDI_Global.ep, i));
    }

    *n_vnis_provided = MPIDI_Global.max_ch4_vnis;

    if (do_av_insert) {
        /* ---------------------------------- */
        /* Get our endpoint name and publish  */
        /* the socket to the KVS              */
        /* ---------------------------------- */
        MPIDI_Global.addrnamelen = FI_NAME_MAX;
        MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_Global.ep, MPIDI_Global.addrname,
                                  &MPIDI_Global.addrnamelen), getname);
        MPIR_Assert(MPIDI_Global.addrnamelen <= FI_NAME_MAX);

        mpi_errno = MPIDU_bc_table_create(rank, size, MPIDI_CH4_Global.node_map[0],
                                          &MPIDI_Global.addrname, MPIDI_Global.addrnamelen, TRUE,
                                          MPIR_CVAR_CH4_ROOTS_ONLY_PMI, &table, NULL);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* -------------------------------- */
        /* Table is constructed.  Map it    */
        /* -------------------------------- */
        if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
            int *node_roots, num_nodes;

            MPIR_NODEMAP_get_node_roots(MPIDI_CH4_Global.node_map[0], size, &node_roots,
                                        &num_nodes);
            mapped_table = (fi_addr_t *) MPL_malloc(num_nodes * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
            MPIDI_OFI_CALL(fi_av_insert
                           (MPIDI_Global.av, table, num_nodes, mapped_table, 0ULL, NULL), avmap);

            for (i = 0; i < num_nodes; i++) {
                MPIDI_OFI_AV(&MPIDIU_get_av(0, node_roots[i])).dest = mapped_table[i];
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
                MPIDI_OFI_AV(&MPIDIU_get_av(0, node_roots[i])).ep_idx = 0;
#else
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
                MPIDI_OFI_AV(&MPIDIU_get_av(0, node_roots[i])).ep_idx = 0;
#endif
#endif
            }
            MPL_free(mapped_table);
            MPL_free(node_roots);
        } else {
            mapped_table = (fi_addr_t *) MPL_malloc(size * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
            MPIDI_OFI_CALL(fi_av_insert(MPIDI_Global.av, table, size, mapped_table, 0ULL, NULL),
                           avmap);

            for (i = 0; i < size; i++) {
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest = mapped_table[i];
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#else
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#endif
#endif
            }
            MPL_free(mapped_table);
            MPIDU_bc_table_destroy(table);
        }
    }

    /* -------------------------------- */
    /* Create the id to object maps     */
    /* -------------------------------- */
    MPIDI_CH4U_map_create(&MPIDI_Global.win_map, MPL_MEM_RMA);

    /* ---------------------------------- */
    /* Initialize Active Message          */
    /* ---------------------------------- */
    mpi_errno = MPIDIG_init(comm_world, comm_self, *n_vnis_provided);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIDI_OFI_ENABLE_AM) {
        /* Maximum possible message size for short message send (=eager send)
         * See MPIDI_OFI_do_am_isend for short/long switching logic */
        MPIR_Assert(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE <= MPIDI_Global.max_send);
        MPIDI_Global.am_buf_pool =
            MPIDI_CH4U_create_buf_pool(MPIDI_OFI_BUF_POOL_NUM, MPIDI_OFI_BUF_POOL_SIZE);

        MPIDI_Global.cq_buffered_dynamic_head = MPIDI_Global.cq_buffered_dynamic_tail = NULL;
        MPIDI_Global.cq_buffered_static_head = MPIDI_Global.cq_buffered_static_tail = 0;
        optlen = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;

        MPIDI_OFI_CALL(fi_setopt(&(MPIDI_Global.ctx[0].rx->fid),
                                 FI_OPT_ENDPOINT,
                                 FI_OPT_MIN_MULTI_RECV, &optlen, sizeof(optlen)), setopt);

        for (i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++) {
            MPIDI_Global.am_bufs[i] = MPL_malloc(MPIDI_OFI_AM_BUFF_SZ, MPL_MEM_BUFFER);
            MPIDI_Global.am_reqs[i].event_id = MPIDI_OFI_EVENT_AM_RECV;
            MPIDI_Global.am_reqs[i].index = i;
            MPIR_Assert(MPIDI_Global.am_bufs[i]);
            MPIDI_OFI_ASSERT_IOVEC_ALIGN(&MPIDI_Global.am_iov[i]);
            MPIDI_Global.am_iov[i].iov_base = MPIDI_Global.am_bufs[i];
            MPIDI_Global.am_iov[i].iov_len = MPIDI_OFI_AM_BUFF_SZ;
            MPIDI_Global.am_msg[i].msg_iov = &MPIDI_Global.am_iov[i];
            MPIDI_Global.am_msg[i].desc = NULL;
            MPIDI_Global.am_msg[i].addr = FI_ADDR_UNSPEC;
            MPIDI_Global.am_msg[i].context = &MPIDI_Global.am_reqs[i].context;
            MPIDI_Global.am_msg[i].iov_count = 1;
            MPIDI_OFI_CALL_RETRY(fi_recvmsg(MPIDI_Global.ctx[0].rx,
                                            &MPIDI_Global.am_msg[i],
                                            FI_MULTI_RECV | FI_COMPLETION), prepost,
                                 MPIDI_OFI_CALL_LOCK, FALSE);
        }

        /* Grow the header handlers down */
        MPIDIG_global.target_msg_cbs[MPIDI_OFI_INTERNAL_HANDLER_CONTROL] =
            MPIDI_OFI_control_handler;
        MPIDIG_global.origin_cbs[MPIDI_OFI_INTERNAL_HANDLER_CONTROL] = NULL;
    }
    OPA_store_int(&MPIDI_Global.am_inflight_inject_emus, 0);
    OPA_store_int(&MPIDI_Global.am_inflight_rma_send_mrs, 0);

#ifndef HAVE_DEBUGGER_SUPPORT
    MPIDI_Global.lw_send_req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    if (MPIDI_Global.lw_send_req == NULL) {
        MPIR_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }
    MPIR_cc_set(&MPIDI_Global.lw_send_req->cc, 0);
#endif

    /* -------------------------------- */
    /* Initialize Dynamic Tasking       */
    /* -------------------------------- */
#if !defined(USE_PMIX_API) && !defined(USE_PMI2_API)
    MPIDI_OFI_conn_manager_init();
    if (spawned) {
        char parent_port[MPIDI_MAX_KVS_VALUE_LEN];
        MPIDI_OFI_PMI_CALL_POP(PMI_KVS_Get(MPIDI_Global.kvsname,
                                           MPIDI_PARENT_PORT_KVSKEY,
                                           parent_port, MPIDI_MAX_KVS_VALUE_LEN), pmi);
        MPIDI_OFI_MPI_CALL_POP(MPID_Comm_connect
                               (parent_port, NULL, 0, comm_world, &MPIR_Process.comm_parent));
        MPIR_Assert(MPIR_Process.comm_parent != NULL);
        MPL_strncpy(MPIR_Process.comm_parent->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
    }
#endif

    mpi_errno = MPIR_Comm_register_hint("eagain", MPIDI_OFI_set_eagain, NULL);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:

    /* -------------------------------- */
    /* Free temporary resources         */
    /* -------------------------------- */
    if (provname) {
        MPL_free(provname);
        hints->fabric_attr->prov_name = NULL;
    }

    if (prov)
        fi_freeinfo(prov);

    fi_freeinfo(hints);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int MPIDI_NM_mpi_finalize_hook(void)
{
    int thr_err = 0, mpi_errno = MPI_SUCCESS;
    int i = 0;
    int barrier[2] = { 0 };
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *comm;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_FINALIZE);

    /* clean dynamic process connections */
    MPIDI_OFI_conn_manager_destroy();

    /* Progress until we drain all inflight RMA send long buffers */
    while (OPA_load_int(&MPIDI_Global.am_inflight_rma_send_mrs) > 0)
        MPIDI_OFI_PROGRESS();

    /* Barrier over allreduce, but force non-immediate send */
    MPIDI_Global.max_buffered_send = 0;
    MPIDI_OFI_MPI_CALL_POP(MPIR_Allreduce(&barrier[0], &barrier[1], 1, MPI_INT,
                                          MPI_SUM, MPIR_Process.comm_world, &errflag));

    /* Progress until we drain all inflight injection emulation requests */
    while (OPA_load_int(&MPIDI_Global.am_inflight_inject_emus) > 0)
        MPIDI_OFI_PROGRESS();
    MPIR_Assert(OPA_load_int(&MPIDI_Global.am_inflight_inject_emus) == 0);

    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        for (i = 0; i < MPIDI_Global.max_ch4_vnis; i++) {
            MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_Global.ctx[i].tx), epclose);
            MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_Global.ctx[i].rx), epclose);
            MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_Global.ctx[i].cq), cqclose);
        }
    }

    /* Close RMA scalable EP. */
    if (MPIDI_Global.rma_sep) {
        /* All transmit contexts on RMA must be closed. */
        MPIR_Assert(utarray_len(MPIDI_Global.rma_sep_idx_array) == MPIDI_Global.max_rma_sep_tx_cnt);
        utarray_free(MPIDI_Global.rma_sep_idx_array);
        MPIDI_OFI_CALL(fi_close(&MPIDI_Global.rma_sep->fid), epclose);
    }
    if (MPIDI_OFI_ENABLE_SHARED_CONTEXTS && MPIDI_Global.rma_stx_ctx != NULL)
        MPIDI_OFI_CALL(fi_close(&MPIDI_Global.rma_stx_ctx->fid), stx_ctx_close);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.ep->fid), epclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.av->fid), avclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.p2p_cq->fid), cqclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.rma_cmpl_cntr->fid), cntrclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.domain->fid), domainclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.fabric->fid), fabricclose);

    fi_freeinfo(MPIDI_Global.prov_use);

    /* --------------------------------------- */
    /* Free comm world addr table              */
    /* --------------------------------------- */
    comm = MPIR_Process.comm_world;
    MPIR_Comm_release_always(comm);

    comm = MPIR_Process.comm_self;
    MPIR_Comm_release_always(comm);

    MPIDIG_finalize();

    MPIDI_CH4U_map_destroy(MPIDI_Global.win_map);

    if (MPIDI_OFI_ENABLE_AM) {
        for (i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++)
            MPL_free(MPIDI_Global.am_bufs[i]);

        MPIDI_CH4R_destroy_buf_pool(MPIDI_Global.am_buf_pool);

        MPIR_Assert(MPIDI_Global.cq_buffered_static_head == MPIDI_Global.cq_buffered_static_tail);
        MPIR_Assert(NULL == MPIDI_Global.cq_buffered_dynamic_head);
    }

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_UTIL_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_FI_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &thr_err);

#ifndef HAVE_DEBUGGER_SUPPORT
    MPIR_Request_free(MPIDI_Global.lw_send_req);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_get_vni_attr(int vni)
{
    MPIR_Assert(0 <= vni && vni < 1);
    return MPIDI_VNI_TX | MPIDI_VNI_RX;
}

static inline void *MPIDI_NM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{

    void *ap;
    ap = MPL_malloc(size, MPL_MEM_USER);
    return ap;
}

static inline int MPIDI_NM_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_free(ptr);

    return mpi_errno;
}

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, bool is_remote)
{
    int avtid = 0, lpid = 0;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else if (is_remote)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LUPID_CREATE(avtid, lpid);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                           char **local_upids)
{
    int mpi_errno = MPI_SUCCESS;
    int i, total_size = 0;
    char *temp_buf = NULL, *curr_ptr = NULL;

    MPIR_CHKPMEM_DECL(2);
    MPIR_CHKLMEM_DECL(1);

    MPIR_CHKPMEM_MALLOC((*local_upid_size), size_t *, comm->local_size * sizeof(size_t),
                        mpi_errno, "local_upid_size", MPL_MEM_ADDRESS);
    MPIR_CHKLMEM_MALLOC(temp_buf, char *, comm->local_size * MPIDI_Global.addrnamelen,
                        mpi_errno, "temp_buf", MPL_MEM_BUFFER);

    for (i = 0; i < comm->local_size; i++) {
        (*local_upid_size)[i] = MPIDI_Global.addrnamelen;
        MPIDI_OFI_CALL(fi_av_lookup(MPIDI_Global.av, MPIDI_OFI_COMM_TO_PHYS(comm, i),
                                    &temp_buf[i * MPIDI_Global.addrnamelen],
                                    &(*local_upid_size)[i]), avlookup);
        total_size += (*local_upid_size)[i];
    }

    MPIR_CHKPMEM_MALLOC((*local_upids), char *, total_size * sizeof(char),
                        mpi_errno, "local_upids", MPL_MEM_BUFFER);
    curr_ptr = (*local_upids);
    for (i = 0; i < comm->local_size; i++) {
        memcpy(curr_ptr, &temp_buf[i * MPIDI_Global.addrnamelen], (*local_upid_size)[i]);
        curr_ptr += (*local_upid_size)[i];
    }

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

static inline int MPIDI_NM_upids_to_lupids(int size,
                                           size_t * remote_upid_size,
                                           char *remote_upids, int **remote_lupids)
{
    int i, mpi_errno = MPI_SUCCESS;
    int *new_avt_procs;
    char **new_upids;
    int n_new_procs = 0;
    int max_n_avts;
    char *curr_upid;
    new_avt_procs = (int *) MPL_malloc(size * sizeof(int), MPL_MEM_ADDRESS);
    new_upids = (char **) MPL_malloc(size * sizeof(char *), MPL_MEM_ADDRESS);
    max_n_avts = MPIDIU_get_max_n_avts();

    curr_upid = remote_upids;
    for (i = 0; i < size; i++) {
        int j, k;
        char tbladdr[FI_NAME_MAX];
        int found = 0;
        size_t sz = 0;

        for (k = 0; k < max_n_avts; k++) {
            if (MPIDIU_get_av_table(k) == NULL) {
                continue;
            }
            for (j = 0; j < MPIDIU_get_av_table(k)->size; j++) {
                sz = MPIDI_Global.addrnamelen;
                MPIDI_OFI_CALL(fi_av_lookup(MPIDI_Global.av, MPIDI_OFI_TO_PHYS(k, j),
                                            &tbladdr, &sz), avlookup);
                if (sz == remote_upid_size[i]
                    && !memcmp(tbladdr, curr_upid, remote_upid_size[i])) {
                    (*remote_lupids)[i] = MPIDIU_LUPID_CREATE(k, j);
                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            new_avt_procs[n_new_procs] = i;
            new_upids[n_new_procs] = curr_upid;
            n_new_procs++;
        }
        curr_upid += remote_upid_size[i];
    }

    /* create new av_table, insert processes */
    if (n_new_procs > 0) {
        int avtid;
        MPIDI_OFI_MPI_CALL_POP(MPIDIU_new_avt(n_new_procs, &avtid));

        for (i = 0; i < n_new_procs; i++) {
            MPIDI_OFI_CALL(fi_av_insert(MPIDI_Global.av, new_upids[i],
                                        1,
                                        (fi_addr_t *) &
                                        MPIDI_OFI_AV(&MPIDIU_get_av(avtid, i)).dest, 0ULL,
                                        NULL), avmap);
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
            MPIDI_OFI_AV(&MPIDIU_get_av(avtid, i)).ep_idx = 0;
#else
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
            MPIDI_OFI_AV(&MPIDIU_get_av(avtid, i)).ep_idx = 0;
#endif
#endif
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\tupids to lupids avtid %d lpid %d mapped to %" PRIu64,
                             avtid, i, MPIDI_OFI_AV(&MPIDIU_get_av(avtid, i)).dest));
            /* highest bit is marked as 1 to indicate this is a new process */
            (*remote_lupids)[new_avt_procs[i]] = MPIDIU_LUPID_CREATE(avtid, i);
            MPIDIU_LUPID_SET_NEW_AVT_MARK((*remote_lupids)[new_avt_procs[i]]);
        }
    }

  fn_exit:
    MPL_free(new_avt_procs);
    MPL_free(new_upids);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                       int size, const int lpids[])
{
    return 0;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_create_endpoint
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_create_endpoint(struct fi_info *prov_use,
                                            struct fid_domain *domain,
                                            struct fid_cq *p2p_cq,
                                            struct fid_cntr *rma_ctr,
                                            struct fid_av *av, struct fid_ep **ep, int index)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_tx_attr tx_attr;
    struct fi_rx_attr rx_attr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_CREATE_ENDPOINT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_CREATE_ENDPOINT);

    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        struct fi_cq_attr cq_attr;

        if (*ep == NULL) {
            /* First call to this function -- set up scalable endpoint */
            MPIDI_OFI_CALL(fi_scalable_ep(domain, prov_use, ep, NULL), ep);
            MPIDI_OFI_CALL(fi_scalable_ep_bind(*ep, &av->fid, 0), bind);
        }

        memset(&cq_attr, 0, sizeof(cq_attr));
        cq_attr.format = FI_CQ_FORMAT_TAGGED;
        MPIDI_OFI_CALL(fi_cq_open(MPIDI_Global.domain,
                                  &cq_attr, &MPIDI_Global.ctx[index].cq, NULL), opencq);

        tx_attr = *prov_use->tx_attr;
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

        MPIDI_OFI_CALL(fi_tx_context(*ep, index, &tx_attr, &MPIDI_Global.ctx[index].tx, NULL), ep);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_Global.ctx[index].tx,
                                  &MPIDI_Global.ctx[index].cq->fid,
                                  FI_SEND | FI_SELECTIVE_COMPLETION), bind);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_Global.ctx[index].tx, &rma_ctr->fid, FI_WRITE | FI_READ),
                       bind);

        rx_attr = *prov_use->rx_attr;
        rx_attr.caps = 0;

        if (MPIDI_OFI_ENABLE_TAGGED)
            rx_attr.caps |= FI_TAGGED;
        if (MPIDI_OFI_ENABLE_DATA)
            rx_attr.caps |= FI_DIRECTED_RECV;

        if (MPIDI_OFI_ENABLE_RMA)
            rx_attr.caps |= FI_RMA | FI_REMOTE_READ | FI_REMOTE_WRITE;
        if (MPIDI_OFI_ENABLE_ATOMICS)
            rx_attr.caps |= FI_ATOMICS;
        rx_attr.caps |= FI_MSG;
        rx_attr.caps |= FI_MULTI_RECV;

        MPIDI_OFI_CALL(fi_rx_context(*ep, index, &rx_attr, &MPIDI_Global.ctx[index].rx, NULL), ep);
        MPIDI_OFI_CALL(fi_ep_bind
                       (MPIDI_Global.ctx[index].rx, &MPIDI_Global.ctx[index].cq->fid, FI_RECV),
                       bind);

        MPIDI_OFI_CALL(fi_enable(*ep), ep_enable);

        MPIDI_OFI_CALL(fi_enable(MPIDI_Global.ctx[index].tx), ep_enable);
        MPIDI_OFI_CALL(fi_enable(MPIDI_Global.ctx[index].rx), ep_enable);
    } else {
        /* ---------------------------------------------------------- */
        /* Bind the CQs, counters,  and AV to the endpoint object     */
        /* ---------------------------------------------------------- */
        /* "Normal Endpoint */
        MPIDI_OFI_CALL(fi_endpoint(domain, prov_use, ep, NULL), ep);
        MPIDI_OFI_CALL(fi_ep_bind(*ep, &p2p_cq->fid, FI_SEND | FI_RECV | FI_SELECTIVE_COMPLETION),
                       bind);
        MPIDI_OFI_CALL(fi_ep_bind(*ep, &rma_ctr->fid, FI_READ | FI_WRITE), bind);
        MPIDI_OFI_CALL(fi_ep_bind(*ep, &av->fid, 0), bind);
        MPIDI_OFI_CALL(fi_enable(*ep), ep_enable);

        /* Copy the normal ep into the first entry for scalable endpoints to
         * allow compile macros to work */
        MPIDI_Global.ctx[0].tx = MPIDI_Global.ctx[0].rx = *ep;
        MPIDI_Global.ctx[0].cq = p2p_cq;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_CREATE_ENDPOINT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline void MPIDI_OFI_dump_providers(struct fi_info *prov)
{
    fprintf(stdout, "Dumping Providers(first=%p):\n", prov);
    while (prov) {
        fprintf(stdout, "%s", fi_tostr(prov, FI_TYPE_INFO));
        prov = prov->next;
    }
}

static inline int MPIDI_OFI_application_hints(int rank)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_DATA: %d", MPIDI_OFI_ENABLE_DATA));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_AV_TABLE: %d", MPIDI_OFI_ENABLE_AV_TABLE));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS: %d",
                     MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_SHARED_CONTEXTS: %d",
                     MPIDI_OFI_ENABLE_SHARED_CONTEXTS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_MR_SCALABLE: %d",
                     MPIDI_OFI_ENABLE_MR_SCALABLE));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_TAGGED: %d", MPIDI_OFI_ENABLE_TAGGED));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_AM: %d", MPIDI_OFI_ENABLE_AM));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_RMA: %d", MPIDI_OFI_ENABLE_RMA));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_FETCH_ATOMIC_IOVECS: %d",
                     MPIDI_OFI_FETCH_ATOMIC_IOVECS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_IOVEC_ALIGN: %d", MPIDI_OFI_IOVEC_ALIGN));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS: %d",
                     MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS: %d",
                     MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_ENABLE_PT2PT_NOPACK: %d",
                     MPIDI_OFI_ENABLE_PT2PT_NOPACK));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_NUM_AM_BUFFERS: %d", MPIDI_OFI_NUM_AM_BUFFERS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_CONTEXT_BITS: %d", MPIDI_OFI_CONTEXT_BITS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_SOURCE_BITS: %d", MPIDI_OFI_SOURCE_BITS));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_OFI_TAG_BITS: %d", MPIDI_OFI_TAG_BITS));

    if (MPIR_CVAR_CH4_OFI_CAPABILITY_SETS_DEBUG && rank == 0) {
        fprintf(stdout, "==== Capability set configuration ====\n");
        fprintf(stdout, "MPIDI_OFI_ENABLE_DATA: %d\n", MPIDI_OFI_ENABLE_DATA);
        fprintf(stdout, "MPIDI_OFI_ENABLE_AV_TABLE: %d\n", MPIDI_OFI_ENABLE_AV_TABLE);
        fprintf(stdout, "MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS: %d\n",
                MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
        fprintf(stdout, "MPIDI_OFI_ENABLE_SHARED_CONTEXTS: %d\n", MPIDI_OFI_ENABLE_SHARED_CONTEXTS);
        fprintf(stdout, "MPIDI_OFI_ENABLE_MR_SCALABLE: %d\n", MPIDI_OFI_ENABLE_MR_SCALABLE);
        fprintf(stdout, "MPIDI_OFI_ENABLE_TAGGED: %d\n", MPIDI_OFI_ENABLE_TAGGED);
        fprintf(stdout, "MPIDI_OFI_ENABLE_AM: %d\n", MPIDI_OFI_ENABLE_AM);
        fprintf(stdout, "MPIDI_OFI_ENABLE_RMA: %d\n", MPIDI_OFI_ENABLE_RMA);
        fprintf(stdout, "MPIDI_OFI_ENABLE_ATOMICS: %d\n", MPIDI_OFI_ENABLE_ATOMICS);
        fprintf(stdout, "MPIDI_OFI_FETCH_ATOMIC_IOVECS: %d\n", MPIDI_OFI_FETCH_ATOMIC_IOVECS);
        fprintf(stdout, "MPIDI_OFI_IOVEC_ALIGN: %d\n", MPIDI_OFI_IOVEC_ALIGN);
        fprintf(stdout, "MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS: %d\n",
                MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS);
        fprintf(stdout, "MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS: %d\n",
                MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS);
        fprintf(stdout, "MPIDI_OFI_ENABLE_PT2PT_NOPACK: %d\n", MPIDI_OFI_ENABLE_PT2PT_NOPACK);
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

    /* Check that the desired number of ranks is possible and abort if not */
    if (MPIDI_OFI_MAX_RANK_BITS < 32 &&
        MPIR_Comm_size(MPIR_Process.comm_world) > (1 << MPIDI_OFI_MAX_RANK_BITS)) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch4|too_many_ranks");
    }

  fn_fail:
    return mpi_errno;
}

static inline int MPIDI_OFI_init_global_settings(const char *prov_name)
{
    /* Seed the global settings values for cases where we are using runtime sets */
    MPIDI_Global.settings.enable_data =
        MPIR_CVAR_CH4_OFI_ENABLE_DATA !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_DATA : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_data :
        MPIR_CVAR_CH4_OFI_ENABLE_DATA;
    MPIDI_Global.settings.enable_av_table =
        MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_av_table :
        MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE;
    MPIDI_Global.settings.enable_scalable_endpoints =
        MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_scalable_endpoints :
        MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS;
    /* If the user doesn't care, then try to use them and fall back if necessary in the RMA init code */
    MPIDI_Global.settings.enable_shared_contexts =
        MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_shared_contexts :
        MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS;
    MPIDI_Global.settings.enable_mr_scalable =
        MPIR_CVAR_CH4_OFI_ENABLE_MR_SCALABLE !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_MR_SCALABLE : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_mr_scalable :
        MPIR_CVAR_CH4_OFI_ENABLE_MR_SCALABLE;
    MPIDI_Global.settings.enable_tagged =
        MPIR_CVAR_CH4_OFI_ENABLE_TAGGED !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_TAGGED : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_tagged :
        MPIR_CVAR_CH4_OFI_ENABLE_TAGGED;
    MPIDI_Global.settings.enable_am =
        MPIR_CVAR_CH4_OFI_ENABLE_AM !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_AM : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_am :
        MPIR_CVAR_CH4_OFI_ENABLE_AM;
    MPIDI_Global.settings.enable_rma =
        MPIR_CVAR_CH4_OFI_ENABLE_RMA !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_RMA : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_rma :
        MPIR_CVAR_CH4_OFI_ENABLE_RMA;
    /* try to enable atomics only when RMA is enabled */
    MPIDI_Global.settings.enable_atomics =
        MPIDI_OFI_ENABLE_RMA ? (MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS !=
                                -1 ? MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS : prov_name ?
                                MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number
                                                    (prov_name)].enable_atomics :
                                MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS) : 0;
    MPIDI_Global.settings.fetch_atomic_iovecs =
        MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS !=
        -1 ? MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].fetch_atomic_iovecs :
        MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS;
    MPIDI_Global.settings.enable_data_auto_progress =
        MPIR_CVAR_CH4_OFI_ENABLE_DATA_AUTO_PROGRESS !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_DATA_AUTO_PROGRESS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_data_auto_progress :
        MPIR_CVAR_CH4_OFI_ENABLE_DATA_AUTO_PROGRESS;
    MPIDI_Global.settings.enable_control_auto_progress =
        MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_control_auto_progress :
        MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS;
    MPIDI_Global.settings.enable_pt2pt_nopack =
        MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK !=
        -1 ? MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].enable_pt2pt_nopack :
        MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK;
    MPIDI_Global.settings.context_bits =
        MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS !=
        -1 ? MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].context_bits :
        MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS;
    MPIDI_Global.settings.source_bits =
        MPIR_CVAR_CH4_OFI_RANK_BITS !=
        -1 ? MPIR_CVAR_CH4_OFI_RANK_BITS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].source_bits :
        MPIR_CVAR_CH4_OFI_RANK_BITS;
    MPIDI_Global.settings.tag_bits =
        MPIR_CVAR_CH4_OFI_TAG_BITS !=
        -1 ? MPIR_CVAR_CH4_OFI_TAG_BITS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].tag_bits :
        MPIR_CVAR_CH4_OFI_TAG_BITS;
    MPIDI_Global.settings.major_version =
        MPIR_CVAR_CH4_OFI_MAJOR_VERSION !=
        -1 ? MPIR_CVAR_CH4_OFI_MAJOR_VERSION : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].major_version :
        MPIR_CVAR_CH4_OFI_MAJOR_VERSION;
    MPIDI_Global.settings.minor_version =
        MPIR_CVAR_CH4_OFI_MINOR_VERSION !=
        -1 ? MPIR_CVAR_CH4_OFI_MINOR_VERSION : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].minor_version :
        MPIR_CVAR_CH4_OFI_MINOR_VERSION;
    MPIDI_Global.settings.num_am_buffers =
        MPIR_CVAR_CH4_OFI_NUM_AM_BUFFERS !=
        -1 ? MPIR_CVAR_CH4_OFI_NUM_AM_BUFFERS : prov_name ?
        MPIDI_OFI_caps_list[MPIDI_OFI_get_set_number(prov_name)].num_am_buffers :
        MPIDI_OFI_NUM_AM_BUFFERS_MINIMAL;
    if (MPIDI_Global.settings.num_am_buffers < 0) {
        MPIDI_Global.settings.num_am_buffers = 0;
    }
    if (MPIDI_Global.settings.num_am_buffers > MPIDI_OFI_MAX_NUM_AM_BUFFERS) {
        MPIDI_Global.settings.num_am_buffers = MPIDI_OFI_MAX_NUM_AM_BUFFERS;
    }
    return MPI_SUCCESS;
}

static inline int MPIDI_OFI_init_hints(struct fi_info *hints)
{
    int fi_version = FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION);
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
    /*           We expect to register all memory up front for use with this    */
    /*           endpoint, so the netmod requires dynamic memory regions        */
    /* ------------------------------------------------------------------------ */
    hints->mode = FI_CONTEXT | FI_ASYNC_IOV | FI_RX_CQ_DATA;    /* We can handle contexts  */

    if (MPIDI_OFI_MAJOR_VERSION != -1 && MPIDI_OFI_MINOR_VERSION != -1)
        fi_version = FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION);

    if (fi_version >= FI_VERSION(1, 5)) {
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
    }

    if (MPIDI_OFI_ENABLE_AM) {
        hints->caps |= FI_MSG;  /* Message Queue apis      */
        hints->caps |= FI_MULTI_RECV;   /* Shared receive buffer   */
    }

    if (MPIDI_OFI_ENABLE_DATA) {
        hints->caps |= FI_DIRECTED_RECV;        /* Match source address    */
        hints->domain_attr->cq_data_size = 4;   /* Minimum size for completion data entry */
    }

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
    if (MPIDI_OFI_ENABLE_MR_SCALABLE)
        hints->domain_attr->mr_mode = FI_MR_SCALABLE;
    else
        hints->domain_attr->mr_mode = FI_MR_BASIC;
    if (FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION) >= FI_VERSION(1, 5)) {
#ifdef FI_RESTRICTED_COMP
        hints->domain_attr->mode = FI_RESTRICTED_COMP;
#endif
    }
    hints->tx_attr->op_flags = FI_COMPLETION;
    hints->tx_attr->msg_order = FI_ORDER_SAS;
    /* direct RMA operations supported only with delivery complete mode,
     * else (AM mode) delivery complete is not required */
    if (MPIDI_OFI_ENABLE_RMA || MPIDI_OFI_ENABLE_ATOMICS) {
        hints->tx_attr->op_flags |= FI_DELIVERY_COMPLETE;
        /* Apply most restricted msg order in hints for RMA ATOMICS. */
        if (MPIDI_OFI_ENABLE_ATOMICS)
            hints->tx_attr->msg_order |= FI_ORDER_RAR | FI_ORDER_RAW | FI_ORDER_WAR | FI_ORDER_WAW;
    }
    hints->tx_attr->comp_order = FI_ORDER_NONE;
    hints->rx_attr->op_flags = FI_COMPLETION;
    hints->rx_attr->total_buffered_recv = 0;    /* FI_RM_ENABLED ensures buffering of unexpected messages */
    hints->ep_attr->type = FI_EP_RDM;
    hints->ep_attr->mem_tag_format = MPIDI_OFI_SOURCE_BITS ?
        /*     PROTOCOL         |  CONTEXT  |        SOURCE         |       TAG          */
        MPIDI_OFI_PROTOCOL_MASK | 0 | MPIDI_OFI_SOURCE_MASK | 0 /* With source bits */ :
        MPIDI_OFI_PROTOCOL_MASK | 0 | 0 | MPIDI_OFI_TAG_MASK /* No source bits */ ;

    return MPI_SUCCESS;
}

#endif /* OFI_INIT_H_INCLUDED */
