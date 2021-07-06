/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

/* There are 4 exposed functions in this file.
 *
 * MPIDI_OFI_init_settings -- copying static settings (matching prov_name)
 * allowing override by MPIR_CVAR_CH4_OFI_ENABLE_xxx
 *
 * MPIDI_OFI_init_hints -- settings + hardcoded defaults -> fi_info
 *
 * MPIDI_OFI_match_provider -- settings compared against fi_info
 *
 * MPIDI_OFI_update_global_settings -- fi_info -> MPIDI_OFI_global.settings
 *
 * Note that there are 3 ways global settings and fi_info interact, with
 * similar logic but with subtle differences.
 *
 * Note the conversion from settings to fi_info and fi_info to settings are
 * incomplete in each way, both contains hard coded logics embedded in
 * MPIDI_OFI_init_settings and MPIDI_OFI_init_hints.
 *
 * Thus, it is a complex business, good luck.
 */

/* Initializes hint structure based MPIDI_OFI_global.settings (or config macros) */
int MPIDI_OFI_init_hints(struct fi_info *hints)
{
    int mpi_errno = MPI_SUCCESS;
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

    if (MPIDI_OFI_get_required_version() >= FI_VERSION(1, 5)) {
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

    if (MPIDI_OFI_ENABLE_DATA) {
        hints->caps |= FI_DIRECTED_RECV;        /* Match source address    */
        hints->domain_attr->cq_data_size = MPIDI_OFI_MIN_CQ_DATA_SIZE;  /* Minimum size for completion data entry */
    }

    if (MPIDI_OFI_ENABLE_TAGGED) {
        hints->caps |= FI_TAGGED;       /* Tag matching interface  */
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
    /* or THREAD_COMPLETION: netmod serializes concurrent accesses to OFI       */
    /*                 objects that share the same completion structure.        */
    /* PROGRESS_AUTO:  request providers that make progress without requiring   */
    /*                 the ADI to dedicate a thread to advance the state        */
    /* FI_DELIVERY_COMPLETE:  RMA operations are visible in remote memory       */
    /* FI_COMPLETION:  Selective completions of RMA ops                         */
    /* FI_EP_RDM:  Reliable datagram                                            */
    /* ------------------------------------------------------------------------ */
    hints->addr_format = FI_FORMAT_UNSPEC;
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE) || (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL || defined(MPIDI_OFI_VNI_USE_DOMAIN))
    hints->domain_attr->threading = FI_THREAD_DOMAIN;
#else
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS) {
            hints->domain_attr->threading = FI_THREAD_SAFE;
        } else {
            hints->domain_attr->threading = FI_THREAD_COMPLETION;
        }
    } else {
        hints->domain_attr->threading = FI_THREAD_DOMAIN;
    }
#endif

    MPIDI_OFI_set_auto_progress(hints);
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->domain_attr->av_type = MPIDI_OFI_ENABLE_AV_TABLE ? FI_AV_TABLE : FI_AV_MAP;

    if (MPIDI_OFI_get_required_version() >= FI_VERSION(1, 5)) {
        hints->domain_attr->mr_mode = 0;
#ifdef FI_RESTRICTED_COMP
        hints->domain_attr->mode = FI_RESTRICTED_COMP;
#endif

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

#ifdef FI_MR_ENDPOINT
        hints->domain_attr->mr_mode |= FI_MR_ENDPOINT;
#endif
    } else {
        /* In old versions FI_MR_BASIC is equivallent to set
         * FI_MR_VIRT_ADDR, FI_MR_PROV_KEY, and FI_MR_ALLOCATED on.
         * FI_MR_SCALABLE is equivallent to all bits off in newer versions.
         */
        MPIR_Assert(MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS == MPIDI_OFI_ENABLE_MR_PROV_KEY);
        MPIR_Assert(MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS == MPIDI_OFI_ENABLE_MR_ALLOCATED);
        if (MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
            hints->domain_attr->mr_mode = FI_MR_BASIC;
        } else {
            hints->domain_attr->mr_mode = FI_MR_SCALABLE;
        }
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

    if (MPIDI_OFI_ENABLE_TRIGGERED) {
        if (MPIDI_OFI_ENABLE_ATOMICS && MPIDI_OFI_ENABLE_TAGGED) {
            /* needs FI_TAGGED FI_DIRECTED_RECV ... */
            hints->caps |=
                FI_TRIGGER | FI_DIRECTED_RECV | FI_RMA_EVENT | FI_READ | FI_WRITE | FI_RECV |
                FI_SEND | FI_REMOTE_READ | FI_REMOTE_WRITE | FI_ATOMICS | FI_RMA;
            hints->domain_attr->mr_mode |= FI_MR_RMA_EVENT;
            hints->domain_attr->data_progress = FI_PROGRESS_AUTO;
        } else {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "Triggered ops cannot be enabled if atomics or tagged is disabled"));
            MPIR_ERR_CHKANDJUMP(1, mpi_errno, MPI_ERR_OTHER, "**ofid_enable_trigger");
        }
    }

    hints->tx_attr->comp_order = FI_ORDER_NONE;
    hints->rx_attr->op_flags = FI_COMPLETION;
    hints->rx_attr->total_buffered_recv = 0;    /* FI_RM_ENABLED ensures buffering of unexpected messages */
    hints->ep_attr->type = FI_EP_RDM;
    hints->ep_attr->mem_tag_format = MPIDI_OFI_SOURCE_BITS ?
        /*     PROTOCOL         |  CONTEXT  |        SOURCE         |       TAG          */
        MPIDI_OFI_PROTOCOL_MASK | 0 | MPIDI_OFI_SOURCE_MASK | 0 /* With source bits */ :
        MPIDI_OFI_PROTOCOL_MASK | 0 | 0 | MPIDI_OFI_TAG_MASK /* No source bits */ ;
  fn_fail:
    return mpi_errno;
}

void MPIDI_OFI_set_auto_progress(struct fi_info *hints)
{
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
}

#define UPDATE_SETTING_BY_CAP(cap, CVAR) \
    p_settings->cap = (CVAR != -1) ? CVAR : prov_caps->cap

void MPIDI_OFI_init_settings(MPIDI_OFI_capabilities_t * p_settings, const char *prov_name)
{
    int prov_idx = MPIDI_OFI_get_set_number(prov_name);
    MPIDI_OFI_capabilities_t *prov_caps = &MPIDI_OFI_caps_list[prov_idx];

    memset(p_settings, 0, sizeof(MPIDI_OFI_capabilities_t));

    /* Seed the settings values for cases where we are using runtime sets */
    UPDATE_SETTING_BY_CAP(enable_data, MPIR_CVAR_CH4_OFI_ENABLE_DATA);
    UPDATE_SETTING_BY_CAP(enable_av_table, MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE);
    UPDATE_SETTING_BY_CAP(enable_scalable_endpoints, MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS);
    /* If the user specifies -1 (=don't care) and the provider supports it, then try to use STX
     * and fall back if necessary in the RMA init code */
    UPDATE_SETTING_BY_CAP(enable_shared_contexts, MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS);

    UPDATE_SETTING_BY_CAP(enable_mr_virt_address, MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS);
    UPDATE_SETTING_BY_CAP(enable_mr_prov_key, MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY);
    UPDATE_SETTING_BY_CAP(enable_mr_allocated, MPIR_CVAR_CH4_OFI_ENABLE_MR_ALLOCATED);
    UPDATE_SETTING_BY_CAP(enable_mr_register_null, MPIR_CVAR_CH4_OFI_ENABLE_MR_REGISTER_NULL);

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
    if (!strcmp(prov_name, "sockets")) {
        UPDATE_SETTING_BY_CAP(enable_triggered, MPIR_CVAR_CH4_OFI_ENABLE_TRIGGERED);
    }
    if (p_settings->num_am_buffers < 0) {
        p_settings->num_am_buffers = 0;
    }
    if (p_settings->num_am_buffers > MPIDI_OFI_MAX_NUM_AM_BUFFERS) {
        p_settings->num_am_buffers = MPIDI_OFI_MAX_NUM_AM_BUFFERS;
    }


    /* Always required settings */
    MPIDI_OFI_global.settings.require_rdm = 1;
    UPDATE_SETTING_BY_CAP(num_optimized_memory_regions,
                          MPIR_CVAR_CH4_OFI_NUM_OPTIMIZED_MEMORY_REGIONS);
    UPDATE_SETTING_BY_CAP(enable_hmem, MPIR_CVAR_CH4_OFI_ENABLE_HMEM);
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

    CHECK_CAP(enable_data, prov->domain_attr->cq_data_size < MPIDI_OFI_MIN_CQ_DATA_SIZE);

    /* From the fi_getinfo manpage: "FI_TAGGED implies the ability to send and receive
     * tagged messages." Therefore no need to specify FI_SEND|FI_RECV.  Moreover FI_SEND
     * and FI_RECV are mutually exclusive, so they should never be set both at the same
     * time. */
    CHECK_CAP(enable_tagged, !(prov->caps & FI_TAGGED));

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

    CHECK_CAP(enable_triggered,
              !(prov->caps & (FI_ATOMICS | FI_RMA | FI_TRIGGER | FI_DIRECTED_RECV | FI_RMA_EVENT |
                              FI_READ | FI_WRITE | FI_RECV | FI_SEND | FI_REMOTE_READ |
                              FI_REMOTE_WRITE)));

    CHECK_CAP(enable_data_auto_progress, !(prov->domain_attr->data_progress & FI_PROGRESS_AUTO));
    CHECK_CAP(require_rdm, prov->ep_attr->type != FI_EP_RDM);

    return score;
}

#define UPDATE_SETTING_BY_INFO(cap, info_cond) \
    MPIDI_OFI_global.settings.cap = MPIDI_OFI_global.settings.cap && info_cond
#define UPDATE_SETTING_BY_INFO_DIRECT(cap, info_cond) \
    MPIDI_OFI_global.settings.cap = !!(info_cond)

void MPIDI_OFI_update_global_settings(struct fi_info *prov)
{
    /* ------------------------------------------------------------------------ */
    /* Set global attributes attributes based on the provider choice            */
    /* ------------------------------------------------------------------------ */
    UPDATE_SETTING_BY_INFO(enable_av_table, prov->domain_attr->av_type == FI_AV_TABLE);
    UPDATE_SETTING_BY_INFO(enable_scalable_endpoints,
                           prov->domain_attr->max_ep_tx_ctx > 1 &&
                           (prov->caps & FI_NAMED_RX_CTX) == FI_NAMED_RX_CTX);
    /* NOTE: As of OFI version 1.5, FI_MR_SCALABLE and FI_MR_BASIC are deprecated.
     * FI_MR_BASIC is equivallent to FI_MR_VIRT_ADDR|FI_MR_ALLOCATED|FI_MR_PROV_KEY */
    UPDATE_SETTING_BY_INFO_DIRECT(enable_mr_virt_address,
                                  prov->domain_attr->mr_mode & (FI_MR_VIRT_ADDR | FI_MR_BASIC));
    UPDATE_SETTING_BY_INFO_DIRECT(enable_mr_prov_key,
                                  prov->domain_attr->mr_mode & (FI_MR_PROV_KEY | FI_MR_BASIC));
    UPDATE_SETTING_BY_INFO_DIRECT(enable_mr_allocated,
                                  prov->domain_attr->mr_mode & (FI_MR_ALLOCATED | FI_MR_BASIC));
    UPDATE_SETTING_BY_INFO_DIRECT(enable_data,
                                  (prov->caps & FI_DIRECTED_RECV) &&
                                  prov->domain_attr->cq_data_size >= MPIDI_OFI_MIN_CQ_DATA_SIZE);
    UPDATE_SETTING_BY_INFO(enable_tagged, prov->caps & FI_TAGGED);
    UPDATE_SETTING_BY_INFO(enable_am,
                           (prov->caps & (FI_MSG | FI_MULTI_RECV)) == (FI_MSG | FI_MULTI_RECV));
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

    /* if CQ data is disabled, make sure we have sufficient source_bits */
    if (!MPIDI_OFI_global.settings.enable_data) {
        if (MPIDI_OFI_global.settings.source_bits == 0) {
            MPIDI_OFI_global.settings.context_bits = 16;
            MPIDI_OFI_global.settings.source_bits = 24;
            MPIDI_OFI_global.settings.tag_bits = 20;
        }
    }

    if (!strcmp(prov->fabric_attr->prov_name, "sockets")) {
        MPIDI_OFI_global.settings.enable_triggered = MPIDI_OFI_global.settings.enable_triggered &&
            (prov->caps & (FI_ATOMICS | FI_RMA | FI_TRIGGER | FI_DIRECTED_RECV | FI_RMA_EVENT |
                           FI_READ | FI_WRITE | FI_RECV | FI_SEND | FI_REMOTE_READ |
                           FI_REMOTE_WRITE));
    }

    if (MPIDI_OFI_global.settings.enable_triggered)
        MPIDI_OFI_global.settings.enable_data_auto_progress = 1;

}
