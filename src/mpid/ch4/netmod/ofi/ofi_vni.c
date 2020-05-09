/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

static int create_vni_context(int vni);
static int destroy_vni_context(int vni);

int MPIDI_OFI_vni_init()
{
    int mpi_errno = MPI_SUCCESS;

    /* Create MPIDI_OFI_global.ctx[0] first  */
    mpi_errno = create_vni_context(0);
    MPIR_ERR_CHECK(mpi_errno);

    /* Creating the additional vni contexts.
     * This code maybe moved to a later stage */
    for (int i = 1; i < MPIDI_OFI_global.num_ctx; i++) {
        mpi_errno = create_vni_context(i);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_vni_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Tearing down endpoints */
    for (int i = 1; i < MPIDI_OFI_global.num_ctx; i++) {
        mpi_errno = destroy_vni_context(i);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* 0th ctx is special, synonymous to global context */
    mpi_errno = destroy_vni_context(0);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest = mapped_table[i];
#if MPIDI_OFI_ENABLE_ENDPOINTS_BITS
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#endif
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
