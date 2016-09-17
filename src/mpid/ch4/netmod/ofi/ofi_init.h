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
#ifndef NETMOD_OFI_INIT_H_INCLUDED
#define NETMOD_OFI_INIT_H_INCLUDED

#include "ofi_impl.h"
#include "mpir_cvars.h"
#include "pmi.h"

static inline int MPIDI_OFI_choose_provider(struct fi_info *prov, struct fi_info **prov_use);
static inline int MPIDI_OFI_create_endpoint(struct fi_info *prov_use,
                                            struct fid_domain *domain,
                                            struct fid_cq *p2p_cq,
                                            struct fid_cntr *rma_ctr,
                                            struct fid_av *av,
                                            struct fid_ep **ep, int index, int do_scalable_ep,
                                            int do_data);

#define MPIDI_OFI_CHOOSE_PROVIDER(prov, prov_use,errstr)                          \
    do {                                                                \
        struct fi_info *p = prov;                                               \
        MPIR_ERR_CHKANDJUMP4(p==NULL, mpi_errno,MPI_ERR_OTHER,"**ofid_addrinfo", \
                             "**ofid_addrinfo %s %d %s %s",__SHORT_FILE__, \
                             __LINE__,FCNAME, errstr);                  \
        MPIDI_OFI_choose_provider(prov,prov_use);                           \
    } while (0);

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_init_generic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_init_generic(int rank,
                                         int size,
                                         int appnum,
                                         int *tag_ub,
                                         MPIR_Comm * comm_world,
                                         MPIR_Comm * comm_self,
                                         int spawned,
                                         int num_contexts,
                                         void **netmod_contexts,
                                         int do_av_table,
                                         int do_scalable_ep,
                                         int do_am,
                                         int do_tagged,
                                         int do_data, int do_stx_rma, int do_mr_scalable)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno, i, fi_version;
    int thr_err = 0, str_errno, maxlen;
    char *table = NULL, *provname = NULL;
    struct fi_info *hints, *prov, *prov_use;
    struct fi_cq_attr cq_attr;
    struct fi_cntr_attr cntr_attr;
    fi_addr_t *mapped_table;
    struct fi_av_attr av_attr;
    char valS[MPIDI_KVSAPPSTRLEN], *val;
    char keyS[MPIDI_KVSAPPSTRLEN];
    size_t optlen;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_INIT);

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
    CH4_COMPILE_TIME_ASSERT(offsetof(struct MPIR_Request, dev.ch4.ch4u.netmod_am.ofi.context) ==
                            offsetof(struct MPIR_Request, dev.ch4.netmod.ofi.context));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devreq_t) >= sizeof(MPIDI_OFI_request_t));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIR_Request) >= sizeof(MPIDI_OFI_win_request_t));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIDI_Devgpid_t) >= sizeof(MPIDI_OFI_gpid_t));
    CH4_COMPILE_TIME_ASSERT(sizeof(MPIR_Context_id_t) * 8 >= MPIDI_OFI_AM_CONTEXT_ID_BITS);

    *tag_ub = (1ULL << MPIDI_OFI_TAG_SHIFT) - 1;

    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_UTIL_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_FI_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &thr_err);

    /* ------------------------------------------------------------------------ */
    /* Hints to filter providers                                                */
    /* See man fi_getinfo for a list                                            */
    /* of all filters                                                           */
    /* mode:  Select capabilities that this netmod will support                 */
    /*        FI_CONTEXT:  This netmod will pass in context into communication  */
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

    /* ------------------------------------------------------------------------ */
    /* fi_allocinfo: allocate and zero an fi_info structure and all related     */
    /* substructures                                                            */
    /* ------------------------------------------------------------------------ */
    hints = fi_allocinfo();
    MPIR_Assert(hints != NULL);

    hints->mode = FI_CONTEXT | FI_ASYNC_IOV;    /* We can handle contexts  */
    hints->caps = 0ULL; /* Tag matching interface  */
    hints->caps |= FI_RMA;      /* RMA(read/write)         */
    hints->caps |= FI_ATOMICS;  /* Atomics capabilities    */

    if (do_tagged) {
        hints->caps |= FI_TAGGED;       /* Tag matching interface  */
    }

    if (do_am) {
        hints->caps |= FI_MSG;  /* Message Queue apis      */
        hints->caps |= FI_MULTI_RECV;   /* Shared receive buffer   */
    }

    if (do_data) {
        hints->caps |= FI_DIRECTED_RECV;        /* Match source address    */
    }

    /* ------------------------------------------------------------------------ */
    /* FI_VERSION provides binary backward and forward compatibility support    */
    /* Specify the version of OFI is coded to, the provider will select struct  */
    /* layouts that are compatible with this version.                           */
    /* ------------------------------------------------------------------------ */
    fi_version = FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION);

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
    hints->domain_attr->control_progress = FI_PROGRESS_MANUAL;
    hints->domain_attr->data_progress = FI_PROGRESS_MANUAL;
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->domain_attr->av_type = do_av_table ? FI_AV_TABLE : FI_AV_MAP;
    hints->domain_attr->mr_mode = do_mr_scalable ? FI_MR_SCALABLE : FI_MR_BASIC;
    hints->tx_attr->op_flags = FI_DELIVERY_COMPLETE | FI_COMPLETION;
    hints->tx_attr->msg_order = FI_ORDER_SAS;
    hints->tx_attr->comp_order = FI_ORDER_NONE;
    hints->rx_attr->op_flags = FI_COMPLETION;
    hints->rx_attr->total_buffered_recv = 0;    /* FI_RM_ENABLED ensures buffering of unexpected messages */
    hints->ep_attr->type = FI_EP_RDM;

    /* ------------------------------------------------------------------------ */
    /* fi_getinfo:  returns information about fabric  services for reaching a   */
    /* remote node or service.  this does not necessarily allocate resources.   */
    /* Pass NULL for name/service because we want a list of providers supported */
    /* ------------------------------------------------------------------------ */
    provname = MPIR_CVAR_OFI_USE_PROVIDER ? (char *) MPL_strdup(MPIR_CVAR_OFI_USE_PROVIDER) : NULL;
    hints->fabric_attr->prov_name = provname;
    MPIDI_OFI_CALL(fi_getinfo(fi_version, NULL, NULL, 0ULL, hints, &prov), addrinfo);
    MPIDI_OFI_CHOOSE_PROVIDER(prov, &prov_use, "No suitable provider provider found");

    MPIDI_Global.prov_use = fi_dupinfo(prov_use);
    MPIR_Assert(MPIDI_Global.prov_use);

    /* ------------------------------------------------------------------------ */
    /* Set global attributes attributes based on the provider choice            */
    /* ------------------------------------------------------------------------ */
    MPIDI_Global.max_buffered_send = prov_use->tx_attr->inject_size;
    MPIDI_Global.max_buffered_write = prov_use->tx_attr->inject_size;
    MPIDI_Global.max_send = prov_use->ep_attr->max_msg_size;
    MPIDI_Global.max_write = prov_use->ep_attr->max_msg_size;
    MPIDI_Global.iov_limit = MIN(prov_use->tx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_Global.rma_iov_limit = MIN(prov_use->tx_attr->rma_iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_Global.max_mr_key_size = prov_use->domain_attr->mr_key_size;

    if (MPIDI_Global.max_mr_key_size >= 8) {
        MPIDI_Global.max_windows_bits = MPIDI_OFI_MAX_WINDOWS_BITS_64;
        MPIDI_Global.max_huge_rma_bits = MPIDI_OFI_MAX_HUGE_RMA_BITS_64;
        MPIDI_Global.max_huge_rmas = MPIDI_OFI_MAX_HUGE_RMAS_64;
        MPIDI_Global.huge_rma_shift = MPIDI_OFI_HUGE_RMA_SHIFT_64;
        MPIDI_Global.context_shift = MPIDI_OFI_CONTEXT_SHIFT_64;
    }
    else if (MPIDI_Global.max_mr_key_size >= 4) {
        MPIDI_Global.max_windows_bits = MPIDI_OFI_MAX_WINDOWS_BITS_32;
        MPIDI_Global.max_huge_rma_bits = MPIDI_OFI_MAX_HUGE_RMA_BITS_32;
        MPIDI_Global.max_huge_rmas = MPIDI_OFI_MAX_HUGE_RMAS_32;
        MPIDI_Global.huge_rma_shift = MPIDI_OFI_HUGE_RMA_SHIFT_32;
        MPIDI_Global.context_shift = MPIDI_OFI_CONTEXT_SHIFT_32;
    }
    else if (MPIDI_Global.max_mr_key_size >= 2) {
        MPIDI_Global.max_windows_bits = MPIDI_OFI_MAX_WINDOWS_BITS_16;
        MPIDI_Global.max_huge_rma_bits = MPIDI_OFI_MAX_HUGE_RMA_BITS_16;
        MPIDI_Global.max_huge_rmas = MPIDI_OFI_MAX_HUGE_RMAS_16;
        MPIDI_Global.huge_rma_shift = MPIDI_OFI_HUGE_RMA_SHIFT_16;
        MPIDI_Global.context_shift = MPIDI_OFI_CONTEXT_SHIFT_16;
    }
    else {
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
    MPIDI_OFI_CALL(fi_cntr_open(MPIDI_Global.domain,    /* In:  Domain Object        */
                                &cntr_attr,     /* In:  Configuration object */
                                &MPIDI_Global.rma_cmpl_cntr,    /* Out: Counter Object       */
                                NULL), openct); /* Context: counter events   */

    /* ------------------------------------------------------------------------ */
    /* Construct:  Address Vector                                               */
    /* ------------------------------------------------------------------------ */

    memset(&av_attr, 0, sizeof(av_attr));


    if (do_av_table) {
        av_attr.type = FI_AV_TABLE;
    }
    else {
        av_attr.type = FI_AV_MAP;
    }
    mapped_table = (fi_addr_t *) MPL_malloc(size * sizeof(fi_addr_t));

    av_attr.rx_ctx_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS;

    MPIDI_OFI_CALL(fi_av_open(MPIDI_Global.domain,      /* In:  Domain Object         */
                              &av_attr, /* In:  Configuration object  */
                              &MPIDI_Global.av, /* Out: AV Object             */
                              NULL), avopen);   /* Context: AV events         */

    /* ------------------------------------------------------------------------ */
    /* Construct:  Shared TX Context for RMA                                    */
    /* ------------------------------------------------------------------------ */
    if (do_stx_rma) {
        int ret;
        struct fi_tx_attr tx_attr;
        memset(&tx_attr, 0, sizeof(tx_attr));
        tx_attr.op_flags = FI_DELIVERY_COMPLETE | FI_COMPLETION;
        MPIDI_OFI_CALL_RETURN(fi_stx_context(MPIDI_Global.domain,
                                             &tx_attr,
                                             &MPIDI_Global.stx_ctx, NULL /* context */), ret);
        if (ret < 0) {
            MPL_DBG_MSG(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        "Failed to create shared TX context for RMA, "
                        "falling back to global EP/counter scheme");
            MPIDI_Global.stx_ctx = NULL;
        }
    }

    /* ------------------------------------------------------------------------ */
    /* Create a transport level communication endpoint.  To use the endpoint,   */
    /* it must be bound to completion counters or event queues and enabled,     */
    /* and the resources consumed by it, such as address vectors, counters,     */
    /* completion queues, etc.                                                  */
    /* ------------------------------------------------------------------------ */
    MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_create_endpoint(prov_use,
                                                     MPIDI_Global.domain,
                                                     MPIDI_Global.p2p_cq,
                                                     MPIDI_Global.rma_cmpl_cntr,
                                                     MPIDI_Global.av,
                                                     &MPIDI_Global.ep, 0, do_scalable_ep,
                                                     do_data));

    /* ---------------------------------- */
    /* Get our endpoint name and publish  */
    /* the socket to the KVS              */
    /* ---------------------------------- */
    MPIDI_Global.addrnamelen = FI_NAME_MAX;
    MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_Global.ep, MPIDI_Global.addrname,
                              &MPIDI_Global.addrnamelen), getname);
    MPIR_Assert(MPIDI_Global.addrnamelen <= FI_NAME_MAX);

    val = valS;
    str_errno = MPL_STR_SUCCESS;
    maxlen = MPIDI_KVSAPPSTRLEN;
    memset(val, 0, maxlen);
    MPIDI_OFI_STR_CALL(MPL_str_add_binary_arg(&val, &maxlen, "OFI", (char *) &MPIDI_Global.addrname,
                                              MPIDI_Global.addrnamelen), buscard_len);
    MPIDI_OFI_PMI_CALL_POP(PMI_KVS_Get_my_name(MPIDI_Global.kvsname, MPIDI_KVSAPPSTRLEN), pmi);

    val = valS;
    sprintf(keyS, "OFI-%d", rank);
    MPIDI_OFI_PMI_CALL_POP(PMI_KVS_Put(MPIDI_Global.kvsname, keyS, val), pmi);
    MPIDI_OFI_PMI_CALL_POP(PMI_KVS_Commit(MPIDI_Global.kvsname), pmi);
    MPIDI_OFI_PMI_CALL_POP(PMI_Barrier(), pmi);

    /* -------------------------------- */
    /* Create our address table from    */
    /* encoded KVS values               */
    /* -------------------------------- */
    table = (char *) MPL_malloc(size * MPIDI_Global.addrnamelen);
    maxlen = MPIDI_KVSAPPSTRLEN;

    for (i = 0; i < size; i++) {
        sprintf(keyS, "OFI-%d", i);
        MPIDI_OFI_PMI_CALL_POP(PMI_KVS_Get(MPIDI_Global.kvsname, keyS, valS, MPIDI_KVSAPPSTRLEN),
                               pmi);
        MPIDI_OFI_STR_CALL(MPL_str_get_binary_arg
                           (valS, "OFI", (char *) &table[i * MPIDI_Global.addrnamelen],
                            MPIDI_Global.addrnamelen, &maxlen), buscard_len);
    }

    /* -------------------------------- */
    /* Table is constructed.  Map it    */
    /* -------------------------------- */
    MPIDI_OFI_CALL(fi_av_insert(MPIDI_Global.av, table, size, mapped_table, 0ULL, NULL), avmap);
    for (i = 0; i < size; i++) {
        MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest = mapped_table[i];
    }
    MPL_free(mapped_table);

    /* -------------------------------- */
    /* Create the id to object maps     */
    /* -------------------------------- */
    MPIDI_OFI_map_create(&MPIDI_Global.win_map);

    /* ---------------------------------- */
    /* Initialize Active Message          */
    /* ---------------------------------- */
    if (do_am) {
        /* Maximum possible message size for short message send (=eager send)
         * See MPIDI_OFI_do_am_isend for short/long switching logic */
        MPIR_Assert(MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE <= MPIDI_Global.max_send);
        MPIDI_Global.am_buf_pool =
            MPIDI_CH4U_create_buf_pool(MPIDI_OFI_BUF_POOL_NUM, MPIDI_OFI_BUF_POOL_SIZE);
        mpi_errno = MPIDI_CH4U_mpi_init(comm_world, comm_self, num_contexts, netmod_contexts);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        slist_init(&MPIDI_Global.cq_buff_list);
        MPIDI_Global.cq_buff_head = MPIDI_Global.cq_buff_tail = 0;
        optlen = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;

        MPIDI_OFI_CALL(fi_setopt(&(MPIDI_OFI_EP_RX_MSG(0)->fid),
                                 FI_OPT_ENDPOINT,
                                 FI_OPT_MIN_MULTI_RECV, &optlen, sizeof(optlen)), setopt);

        for (i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++) {
            MPIDI_Global.am_bufs[i] = MPL_malloc(MPIDI_OFI_AM_BUFF_SZ);
            MPIDI_Global.am_reqs[i].event_id = MPIDI_OFI_EVENT_AM_RECV;
            MPIDI_Global.am_reqs[i].index = i;
            MPIR_Assert(MPIDI_Global.am_bufs[i]);
            MPIDI_Global.am_iov[i].iov_base = MPIDI_Global.am_bufs[i];
            MPIDI_Global.am_iov[i].iov_len = MPIDI_OFI_AM_BUFF_SZ;
            MPIDI_Global.am_msg[i].msg_iov = &MPIDI_Global.am_iov[i];
            MPIDI_Global.am_msg[i].desc = NULL;
            MPIDI_Global.am_msg[i].addr = FI_ADDR_UNSPEC;
            MPIDI_Global.am_msg[i].context = &MPIDI_Global.am_reqs[i].context;
            MPIDI_Global.am_msg[i].iov_count = 1;
            MPIDI_OFI_CALL_RETRY(fi_recvmsg(MPIDI_OFI_EP_RX_MSG(0),
                                            &MPIDI_Global.am_msg[i],
                                            FI_MULTI_RECV | FI_COMPLETION), prepost,
                                 MPIDI_OFI_CALL_LOCK);
        }

        /* Grow the header handlers down */
        MPIDI_Global.am_handlers[MPIDI_OFI_INTERNAL_HANDLER_CONTROL] = MPIDI_OFI_control_handler;
        MPIDI_Global.am_isend_cmpl_handlers[MPIDI_OFI_INTERNAL_HANDLER_CONTROL] = NULL;
    }
    OPA_store_int(&MPIDI_Global.am_inflight_inject_emus, 0);
    OPA_store_int(&MPIDI_Global.am_inflight_rma_send_mrs, 0);

    /* max_inject_size is temporarily set to 1 inorder to avoid deadlock in
     * shm initialization since PMI_Barrier does not call progress and flush its injects */
    MPIDI_Global.max_buffered_send = 1;
    MPIDI_Global.max_buffered_write = 1;

    MPIDI_Global.max_buffered_send = prov_use->tx_attr->inject_size;
    MPIDI_Global.max_buffered_write = prov_use->tx_attr->inject_size;

    MPIR_Datatype_init_names();
    MPIDI_OFI_index_datatypes();

    /* -------------------------------- */
    /* Initialize Dynamic Tasking       */
    /* -------------------------------- */
    if (spawned) {
        char parent_port[MPIDI_MAX_KVS_VALUE_LEN];
        MPIDI_OFI_PMI_CALL_POP(PMI_KVS_Get(MPIDI_Global.kvsname,
                                           MPIDI_PARENT_PORT_KVSKEY,
                                           parent_port, MPIDI_MAX_KVS_VALUE_LEN), pmi);
        MPIDI_OFI_MPI_CALL_POP(MPIDI_Comm_connect
                               (parent_port, NULL, 0, comm_world, &MPIR_Process.comm_parent));
        MPIR_Assert(MPIR_Process.comm_parent != NULL);
        MPL_strncpy(MPIR_Process.comm_parent->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
    }

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

    if (table)
        MPL_free(table);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_INIT);
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
                                         int *tag_ub,
                                         MPIR_Comm * comm_world,
                                         MPIR_Comm * comm_self,
                                         int spawned, int num_contexts, void **netmod_contexts)
{
    int mpi_errno;
    mpi_errno = MPIDI_OFI_init_generic(rank, size, appnum, tag_ub, comm_world,
                                       comm_self, spawned, num_contexts,
                                       netmod_contexts,
                                       MPIDI_OFI_ENABLE_AV_TABLE,
                                       MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS,
                                       MPIDI_OFI_ENABLE_AM,
                                       MPIDI_OFI_ENABLE_TAGGED,
                                       MPIDI_OFI_ENABLE_DATA,
                                       MPIDI_OFI_ENABLE_STX_RMA, MPIDI_OFI_ENABLE_MR_SCALABLE);
    return mpi_errno;
}




static inline int MPIDI_OFI_finalize_generic(int do_scalable_ep, int do_am, int do_stx_rma)
{
    int thr_err = 0, mpi_errno = MPI_SUCCESS;
    int i = 0;
    int barrier[2] = { 0 };
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm *comm;

    /* Progress until we drain all inflight RMA send long buffers */
    while (OPA_load_int(&MPIDI_Global.am_inflight_rma_send_mrs) > 0)
        MPIDI_OFI_PROGRESS();

    /* Barrier over allreduce, but force non-immediate send */
    MPIDI_Global.max_buffered_send = 0;
    MPIDI_OFI_MPI_CALL_POP(MPIR_Allreduce_impl(&barrier[0], &barrier[1], 1, MPI_INT,
                                               MPI_SUM, MPIR_Process.comm_world, &errflag));

    /* Progress until we drain all inflight injection emulation requests */
    while (OPA_load_int(&MPIDI_Global.am_inflight_inject_emus) > 0)
        MPIDI_OFI_PROGRESS();
    MPIR_Assert(OPA_load_int(&MPIDI_Global.am_inflight_inject_emus) == 0);

    if (do_scalable_ep) {
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_EP_TX_TAG(0)), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_EP_TX_RMA(0)), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_EP_TX_MSG(0)), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_EP_TX_CTR(0)), epclose);

        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_EP_RX_TAG(0)), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_EP_RX_RMA(0)), epclose);
        MPIDI_OFI_CALL(fi_close((fid_t) MPIDI_OFI_EP_RX_MSG(0)), epclose);
    }

    if (do_stx_rma && MPIDI_Global.stx_ctx != NULL)
        MPIDI_OFI_CALL(fi_close(&MPIDI_Global.stx_ctx->fid), stx_ctx_close);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.ep->fid), epclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.av->fid), avclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.p2p_cq->fid), cqclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.rma_cmpl_cntr->fid), cqclose);
    MPIDI_OFI_CALL(fi_close(&MPIDI_Global.domain->fid), domainclose);

    fi_freeinfo(MPIDI_Global.prov_use);

    /* --------------------------------------- */
    /* Free comm world addr table              */
    /* --------------------------------------- */
    comm = MPIR_Process.comm_world;
    MPIR_Comm_release_always(comm);

    comm = MPIR_Process.comm_self;
    MPIR_Comm_release_always(comm);

    MPIDI_CH4U_mpi_finalize();

    MPIDI_OFI_map_destroy(MPIDI_Global.win_map);

    if (do_am) {
        for (i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++)
            MPL_free(MPIDI_Global.am_bufs[i]);

        MPIDI_CH4R_destroy_buf_pool(MPIDI_Global.am_buf_pool);

        MPIR_Assert(MPIDI_Global.cq_buff_head == MPIDI_Global.cq_buff_tail);
        MPIR_Assert(slist_empty(&MPIDI_Global.cq_buff_list));
    }

    PMI_Finalize();

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_UTIL_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_FI_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &thr_err);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_finalize_hook(void)
{
    return MPIDI_OFI_finalize_generic(MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS,
                                      MPIDI_OFI_ENABLE_AM, MPIDI_OFI_ENABLE_STX_RMA);
}

static inline void *MPIDI_NM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{

    void *ap;
    ap = MPL_malloc(size);
    return ap;
}

static inline int MPIDI_NM_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_free(ptr);

    return mpi_errno;
}

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, MPL_bool is_remote)
{
    int avtid = 0, lpid = 0;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else if (is_remote)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LPID_CREATE(avtid, lpid);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_gpid_get(MPIR_Comm * comm_ptr, int rank, MPIR_Gpid * gpid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(rank < comm_ptr->local_size);
    size_t sz = sizeof(MPIDI_OFI_GPID(gpid).addr);
    MPIDI_OFI_CALL(fi_av_lookup(MPIDI_Global.av, MPIDI_OFI_COMM_TO_PHYS(comm_ptr, rank),
                                &MPIDI_OFI_GPID(gpid).addr, &sz), avlookup);
    MPIR_Assert(sz <= sizeof(MPIDI_OFI_GPID(gpid).addr));
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_getallincomm(MPIR_Comm * comm_ptr,
                                        int local_size, MPIR_Gpid local_gpids[], int *singleAVT)
{
    int i;

    for (i = 0; i < comm_ptr->local_size; i++)
        MPIDI_GPID_Get(comm_ptr, i, &local_gpids[i]);

    return 0;
}

static inline int MPIDI_NM_gpid_tolpidarray_generic(int size,
                                                    MPIR_Gpid gpid[], int lpid[], int use_av_table)
{
    int i, mpi_errno = MPI_SUCCESS;
    int *new_avt_procs;
    int n_new_procs = 0;
    int max_n_avts;
    new_avt_procs = (int *) MPL_malloc(size * sizeof(int));
    max_n_avts = MPIDIU_get_max_n_avts();

    for (i = 0; i < size; i++) {
        int j, k;
        char tbladdr[FI_NAME_MAX];
        int found = 0;

        for (k = 0; k < max_n_avts; k++) {
            if (MPIDIU_get_av_table(k) == NULL) {
                continue;
            }
            for (j = 0; j < MPIDIU_get_av_table(k)->size; j++) {
                size_t sz = sizeof(MPIDI_OFI_GPID(&gpid[i]).addr);
                MPIDI_OFI_CALL(fi_av_lookup
                               (MPIDI_Global.av, MPIDI_OFI_TO_PHYS(k, j), &tbladdr, &sz), avlookup);
                MPIR_Assert(sz <= sizeof(MPIDI_OFI_GPID(&gpid[i]).addr));

                if (!memcmp(tbladdr, MPIDI_OFI_GPID(&gpid[i]).addr, sz)) {
                    lpid[i] = MPIDIU_LPID_CREATE(k, j);
                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            new_avt_procs[n_new_procs] = i;
            n_new_procs++;
        }
    }

    /* create new av_table, insert processes */
    if (n_new_procs > 0) {
        int avtid;
        MPIDIU_new_avt(n_new_procs, &avtid);

        for (i = 0; i < n_new_procs; i++) {
            if (use_av_table) { /* logical addressing */
                MPIDI_OFI_CALL(fi_av_insert
                               (MPIDI_Global.av, &MPIDI_OFI_GPID(&gpid[new_avt_procs[i]]).addr, 1,
                                NULL, 0ULL, NULL), avmap);
                /* FIXME: get logical address */
            }
            else {
                MPIDI_OFI_CALL(fi_av_insert
                               (MPIDI_Global.av, &MPIDI_OFI_GPID(&gpid[new_avt_procs[i]]).addr, 1,
                                (fi_addr_t *) & MPIDI_OFI_AV(&MPIDIU_get_av(avtid, i)).dest, 0ULL,
                                NULL), avmap);
            }
            /* highest bit is marked as 1 to indicate this is a new process */
            lpid[i] = MPIDIU_LPID_CREATE(avtid, i);
            MPIDIU_LPID_SET_NEW_AVT_MARK(lpid[i]);
        }
    }


  fn_exit:
    MPL_free(new_avt_procs);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_gpid_tolpidarray(int size, MPIR_Gpid gpid[], int lpid[])
{
    return MPIDI_NM_gpid_tolpidarray_generic(size, gpid, lpid, MPIDI_OFI_ENABLE_AV_TABLE);
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
                                            struct fid_av *av,
                                            struct fid_ep **ep, int index, int do_scalable_ep,
                                            int do_data)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_tx_attr tx_attr;
    struct fi_rx_attr rx_attr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_CREATE_ENDPOINT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_CREATE_ENDPOINT);

    if (do_scalable_ep) {
        int idx_off;
        MPIDI_OFI_CALL(fi_scalable_ep(domain, prov_use, ep, NULL), ep);
        MPIDI_OFI_CALL(fi_scalable_ep_bind(*ep, &av->fid, 0), bind);

        idx_off = index * 4;
        MPIDI_Global.ctx[index].ctx_offset = idx_off;

        tx_attr = *prov_use->tx_attr;
        tx_attr.caps = FI_TAGGED;
        tx_attr.op_flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        MPIDI_OFI_CALL(fi_tx_context(*ep, idx_off, &tx_attr, &MPIDI_OFI_EP_TX_TAG(index), NULL), ep);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_TX_TAG(index), &p2p_cq->fid, FI_SEND | FI_SELECTIVE_COMPLETION), bind);

        tx_attr = *prov_use->tx_attr;
        tx_attr.caps = FI_RMA;
        tx_attr.caps |= FI_ATOMICS;
        tx_attr.op_flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        MPIDI_OFI_CALL(fi_tx_context(*ep, idx_off + 1, &tx_attr, &MPIDI_OFI_EP_TX_RMA(index), NULL),
                       ep);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_TX_RMA(index), &rma_ctr->fid, FI_WRITE | FI_READ),
                       bind);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_TX_RMA(index), &p2p_cq->fid, FI_SEND | FI_SELECTIVE_COMPLETION), bind);

        tx_attr = *prov_use->tx_attr;
        tx_attr.caps = FI_MSG;
        tx_attr.op_flags = FI_COMPLETION | FI_DELIVERY_COMPLETE;
        MPIDI_OFI_CALL(fi_tx_context(*ep, idx_off + 2, &tx_attr, &MPIDI_OFI_EP_TX_MSG(index), NULL),
                       ep);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_TX_MSG(index), &p2p_cq->fid, FI_SEND | FI_SELECTIVE_COMPLETION), bind);

        tx_attr = *prov_use->tx_attr;
        tx_attr.caps = FI_RMA;
        tx_attr.caps |= FI_ATOMICS;
        tx_attr.op_flags = FI_DELIVERY_COMPLETE;
        MPIDI_OFI_CALL(fi_tx_context(*ep, idx_off + 3, &tx_attr, &MPIDI_OFI_EP_TX_CTR(index), NULL),
                       ep);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_TX_CTR(index), &rma_ctr->fid, FI_WRITE | FI_READ),
                       bind);
        /* We need to bind to the CQ if the progress mode is manual.
           Otherwise fi_cq_read would not make progress on this context and potentially leads to a deadlock. */
        if (prov_use->domain_attr->data_progress == FI_PROGRESS_MANUAL)
            MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_TX_CTR(index), &p2p_cq->fid, FI_SEND | FI_SELECTIVE_COMPLETION), bind);

        rx_attr = *prov_use->rx_attr;
        rx_attr.caps = FI_TAGGED;
        if (do_data)
            rx_attr.caps |= FI_DIRECTED_RECV;
        rx_attr.op_flags = 0;
        MPIDI_OFI_CALL(fi_rx_context(*ep, idx_off, &rx_attr, &MPIDI_OFI_EP_RX_TAG(index), NULL), ep);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_RX_TAG(index), &p2p_cq->fid, FI_RECV), bind);

        rx_attr = *prov_use->rx_attr;
        rx_attr.caps = FI_RMA;
        rx_attr.caps |= FI_ATOMICS;
        rx_attr.op_flags = 0;
        MPIDI_OFI_CALL(fi_rx_context(*ep, idx_off + 1, &rx_attr, &MPIDI_OFI_EP_RX_RMA(index), NULL),
                       ep);

        /* Note:  This bind should cause the "passive target" rx context to never generate an event
         * We need this bind for manual progress to ensure that progress is made on the
         * rx_ctr or rma operations during completion queue reads */
        if (prov_use->domain_attr->data_progress == FI_PROGRESS_MANUAL)
            MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_RX_RMA(index), &p2p_cq->fid,
                                      FI_SEND | FI_RECV | FI_SELECTIVE_COMPLETION), bind);

        rx_attr = *prov_use->rx_attr;
        rx_attr.caps = FI_MSG;
        rx_attr.caps |= FI_MULTI_RECV;
        if (do_data)
            rx_attr.caps |= FI_DIRECTED_RECV;
        rx_attr.op_flags = FI_MULTI_RECV;
        MPIDI_OFI_CALL(fi_rx_context(*ep, idx_off + 2, &rx_attr, &MPIDI_OFI_EP_RX_MSG(index), NULL),
                       ep);
        MPIDI_OFI_CALL(fi_ep_bind(MPIDI_OFI_EP_RX_MSG(index), &p2p_cq->fid, FI_RECV), bind);

        MPIDI_OFI_CALL(fi_enable(*ep), ep_enable);

        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_EP_TX_TAG(index)), ep_enable);
        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_EP_TX_RMA(index)), ep_enable);
        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_EP_TX_MSG(index)), ep_enable);
        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_EP_TX_CTR(index)), ep_enable);

        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_EP_RX_TAG(index)), ep_enable);
        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_EP_RX_RMA(index)), ep_enable);
        MPIDI_OFI_CALL(fi_enable(MPIDI_OFI_EP_RX_MSG(index)), ep_enable);
    }
    else {
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
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_CREATE_ENDPOINT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_choose_provider(struct fi_info *prov, struct fi_info **prov_use)
{
    struct fi_info *p = prov;
    int i = 0;
    *prov_use = prov;

    if (MPIR_CVAR_OFI_DUMP_PROVIDERS) {
        fprintf(stdout, "Dumping Providers(first=%p):\n", prov);

        while (p) {
            fprintf(stdout, "%s", fi_tostr(p, FI_TYPE_INFO));
            p = p->next;
        }
    }

    return i;
}

#endif /* NETMOD_OFI_INIT_H_INCLUDED */
