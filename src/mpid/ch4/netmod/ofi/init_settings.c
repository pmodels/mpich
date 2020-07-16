/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"

/* Initializes hint structure based MPIDI_OFI_global.settings (or config macros) */
void MPIDI_OFI_init_hints(struct fi_info *hints)
{
    int ofi_version = MPIDI_OFI_get_fi_version();
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
            hints->domain_attr->mr_mode |= FI_MR_VIRT_ADDR | FI_MR_ALLOCATED;
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

#define UPDATE_SETTING_BY_CAP(cap, CVAR) \
    p_settings->cap = (CVAR != -1) ? CVAR : prov_caps->cap

void MPIDI_OFI_init_settings(MPIDI_OFI_capabilities_t * p_settings, const char *prov_name)
{
    int prov_idx = MPIDI_OFI_get_set_number(prov_name);
    MPIDI_OFI_capabilities_t *prov_caps = &MPIDI_OFI_caps_list[prov_idx];

    memset(p_settings, 0, sizeof(MPIDI_OFI_capabilities_t));

    /* Seed the settings values for cases where we are using runtime sets */
    UPDATE_SETTING_BY_CAP(enable_av_table, MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE);
    UPDATE_SETTING_BY_CAP(enable_scalable_endpoints, MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS);
    /* If the user specifies -1 (=don't care) and the provider supports it, then try to use STX
     * and fall back if necessary in the RMA init code */
    UPDATE_SETTING_BY_CAP(enable_shared_contexts, MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS);

    /* As of OFI version 1.5, FI_MR_SCALABLE and FI_MR_BASIC are deprecated. Internally, we now use
     * FI_MR_VIRT_ADDRESS and FI_MR_PROV_KEY so set them appropriately depending on the OFI version
     * being used. */
    if (MPIDI_OFI_get_fi_version() < FI_VERSION(1, 5)) {
        /* If the OFI library is 1.5 or less, query whether or not to use FI_MR_SCALABLE and set
         * FI_MR_VIRT_ADDRESS and FI_MR_PROV_KEY as the opposite values. */
        UPDATE_SETTING_BY_CAP(enable_mr_scalable, MPIR_CVAR_CH4_OFI_ENABLE_MR_SCALABLE);
        MPIDI_OFI_global.settings.enable_mr_virt_address =
            !MPIDI_OFI_global.settings.enable_mr_scalable;
        MPIDI_OFI_global.settings.enable_mr_prov_key =
            !MPIDI_OFI_global.settings.enable_mr_scalable;
    } else {
        UPDATE_SETTING_BY_CAP(enable_mr_virt_address, MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS);
        UPDATE_SETTING_BY_CAP(enable_mr_prov_key, MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY);
    }
    UPDATE_SETTING_BY_CAP(enable_tagged, MPIR_CVAR_CH4_OFI_ENABLE_TAGGED);
    UPDATE_SETTING_BY_CAP(enable_am, MPIR_CVAR_CH4_OFI_ENABLE_AM);
    UPDATE_SETTING_BY_CAP(enable_rma, MPIR_CVAR_CH4_OFI_ENABLE_RMA);
    /* try to enable atomics only when RMA is enabled */
    if (p_settings->enable_rma) {
        UPDATE_SETTING_BY_CAP(enable_atomics, MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS);
    } else {
        p_settings->enable_atomics = 0;
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
    if (p_settings->num_am_buffers < 0) {
        p_settings->num_am_buffers = 0;
    }
    if (p_settings->num_am_buffers > MPIDI_OFI_MAX_NUM_AM_BUFFERS) {
        p_settings->num_am_buffers = MPIDI_OFI_MAX_NUM_AM_BUFFERS;
    }

    /* Always required settings */
    p_settings->require_rdm = 1;
}

#define CHECK_CAP(setting, cond_bad) \
    do { \
        bool bad = cond_bad; \
        if (minimal->setting && bad) { \
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, \
                            (MPL_DBG_FDEST, "provider failed minimal setting " #setting)); \
            return 0; \
        } \
        if (optimal->setting && !bad) { \
            score++; \
        } \
    } while (0)

int MPIDI_OFI_match_provider(struct fi_info *prov,
                             MPIDI_OFI_capabilities_t * optimal, MPIDI_OFI_capabilities_t * minimal)
{
    int score = 0;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, "Provider name: %s",
                                                     prov->fabric_attr->prov_name));

#ifdef MPIDI_CH4_OFI_SKIP_IPV6
    if (prov->addr_format == FI_SOCKADDR_IN6) {
        return 0;
    }
#endif
    CHECK_CAP(enable_scalable_endpoints,
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
    CHECK_CAP(enable_tagged, !(prov->caps & FI_TAGGED) || prov->domain_attr->cq_data_size < 4);

    CHECK_CAP(enable_am, (prov->caps & (FI_MSG | FI_MULTI_RECV)) != (FI_MSG | FI_MULTI_RECV));

    CHECK_CAP(enable_rma, !(prov->caps & FI_RMA));
#ifdef FI_HMEM
    CHECK_CAP(enable_hmem, !(prov->caps & FI_HMEM));
#endif
    uint64_t msg_order = MPIDI_OFI_ATOMIC_ORDER_FLAGS;
    CHECK_CAP(enable_atomics,
              !(prov->caps & FI_ATOMICS) || (prov->tx_attr->msg_order & msg_order) != msg_order);

    CHECK_CAP(enable_control_auto_progress,
              !(prov->domain_attr->control_progress & FI_PROGRESS_AUTO));

    CHECK_CAP(enable_data_auto_progress, !(prov->domain_attr->data_progress & FI_PROGRESS_AUTO));

    CHECK_CAP(require_rdm, prov->ep_attr->type != FI_EP_RDM);

    return score;
}

#define UPDATE_SETTING_BY_INFO(cap, info_cond) \
    MPIDI_OFI_global.settings.cap = MPIDI_OFI_global.settings.cap && info_cond

void MPIDI_OFI_update_global_settings(struct fi_info *prov)
{
    /* ------------------------------------------------------------------------ */
    /* Set global attributes attributes based on the provider choice            */
    /* ------------------------------------------------------------------------ */
    UPDATE_SETTING_BY_INFO(enable_av_table, prov->domain_attr->av_type == FI_AV_TABLE);
    UPDATE_SETTING_BY_INFO(enable_scalable_endpoints,
                           prov->domain_attr->max_ep_tx_ctx > 1 &&
                           (prov->caps & FI_NAMED_RX_CTX) == FI_NAMED_RX_CTX);
    /* As of OFI version 1.5, FI_MR_SCALABLE and FI_MR_BASIC are deprecated. Internally, we now use
     * FI_MR_VIRT_ADDRESS and FI_MR_PROV_KEY so set them appropriately depending on the OFI version
     * being used. */
    if (MPIDI_OFI_get_fi_version() < FI_VERSION(1, 5)) {
        /* If the OFI library is 1.5 or less, query whether or not to use FI_MR_SCALABLE and set
         * FI_MR_VIRT_ADDRESS and FI_MR_PROV_KEY as the opposite values. */
        UPDATE_SETTING_BY_INFO(enable_mr_virt_address,
                               prov->domain_attr->mr_mode != FI_MR_SCALABLE);
        UPDATE_SETTING_BY_INFO(enable_mr_prov_key, prov->domain_attr->mr_mode != FI_MR_SCALABLE);
    } else {
        UPDATE_SETTING_BY_INFO(enable_mr_virt_address,
                               prov->domain_attr->mr_mode & FI_MR_VIRT_ADDR);
        UPDATE_SETTING_BY_INFO(enable_mr_prov_key, prov->domain_attr->mr_mode & FI_MR_PROV_KEY);
    }
    UPDATE_SETTING_BY_INFO(enable_tagged,
                           (prov->caps & FI_TAGGED) &&
                           (prov->caps & FI_DIRECTED_RECV) &&
                           (prov->domain_attr->cq_data_size >= 4));
    UPDATE_SETTING_BY_INFO(enable_am,
                           (prov->caps & (FI_MSG | FI_MULTI_RECV | FI_READ)) ==
                           (FI_MSG | FI_MULTI_RECV | FI_READ));
    UPDATE_SETTING_BY_INFO(enable_rma, prov->caps & FI_RMA);
    UPDATE_SETTING_BY_INFO(enable_atomics, prov->caps & FI_ATOMICS);
#ifdef FI_HMEM
    UPDATE_SETTING_BY_INFO(enable_hmem, prov->caps & FI_HMEM);
#endif
    UPDATE_SETTING_BY_INFO(enable_data_auto_progress,
                           prov->domain_attr->data_progress & FI_PROGRESS_AUTO);
    UPDATE_SETTING_BY_INFO(enable_control_auto_progress,
                           prov->domain_attr->control_progress & FI_PROGRESS_AUTO);

    if (MPIDI_OFI_global.settings.enable_scalable_endpoints) {
        MPIDI_OFI_global.settings.max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_SCALABLE;
        MPIDI_OFI_global.settings.max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_SCALABLE;
    } else {
        MPIDI_OFI_global.settings.max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_REGULAR;
        MPIDI_OFI_global.settings.max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR;
    }

    /* update additional global settings (that are not used for provider selection) */
    MPIDI_OFI_global.max_buffered_send = prov->tx_attr->inject_size;
    MPIDI_OFI_global.max_buffered_write = prov->tx_attr->inject_size;
    MPIDI_OFI_global.max_msg_size = MPL_MIN(prov->ep_attr->max_msg_size, MPIR_AINT_MAX);
    MPIDI_OFI_global.max_order_raw = prov->ep_attr->max_order_raw_size;
    MPIDI_OFI_global.max_order_war = prov->ep_attr->max_order_war_size;
    MPIDI_OFI_global.max_order_waw = prov->ep_attr->max_order_waw_size;
    MPIDI_OFI_global.tx_iov_limit = MIN(prov->tx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.rx_iov_limit = MIN(prov->rx_attr->iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.rma_iov_limit = MIN(prov->tx_attr->rma_iov_limit, MPIDI_OFI_IOV_MAX);
    MPIDI_OFI_global.max_mr_key_size = prov->domain_attr->mr_key_size;
}

void MPIDI_OFI_dump_global_settings(void)
{
    fprintf(stdout, "==== Capability set configuration ====\n");
    fprintf(stdout, "libfabric provider: %s\n", MPIDI_OFI_global.prov_use->fabric_attr->prov_name);
    fprintf(stdout, "MPIDI_OFI_ENABLE_AV_TABLE: %d\n", MPIDI_OFI_ENABLE_AV_TABLE);
    fprintf(stdout, "MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS: %d\n",
            MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_SHARED_CONTEXTS: %d\n", MPIDI_OFI_ENABLE_SHARED_CONTEXTS);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_SCALABLE: %d\n", MPIDI_OFI_ENABLE_MR_SCALABLE);
    fprintf(stdout, "MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS: %d\n", MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS);
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
