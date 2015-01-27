/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "ofi_impl.h"

static inline int dump_and_choose_providers(info_t * prov, info_t ** prov_use);
static inline int compile_time_checking();

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_init)
int MPID_nem_ofi_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int ret, fi_version, i, len, pmi_errno;
    int mpi_errno = MPI_SUCCESS;
    info_t hints, *prov_tagged, *prov_use;
    cq_attr_t cq_attr;
    av_attr_t av_attr;
    char kvsname[OFI_KVSAPPSTRLEN], key[OFI_KVSAPPSTRLEN], bc[OFI_KVSAPPSTRLEN];
    char *my_bc, *addrs, *null_addr;
    fi_addr_t *fi_addrs = NULL;
    MPIDI_VC_t *vc;

    BEGIN_FUNC(FCNAME);
    MPIU_CHKLMEM_DECL(2);

    compile_time_checking();
    /* ------------------------------------------------------------------------ */
    /* Hints to filter providers                                                */
    /* See man fi_getinfo for a list                                            */
    /* of all filters                                                           */
    /* mode:  Select capabilities netmod is prepared to support.                */
    /*        In this case, netmod will pass in context into                    */
    /*        communication calls.                                              */
    /*        Note that we do not fill in FI_LOCAL_MR, which means this netmod  */
    /*        does not support exchange of memory regions on communication calls */
    /*        OFI requires that all communication calls use a registered mr     */
    /*        but in our case this netmod is written to only support transfers  */
    /*        on a dynamic memory region that spans all of memory.  So, we do   */
    /*        not set the FI_LOCAL_MR mode bit, and we set the FI_DYNAMIC_MR    */
    /*        bit to tell OFI our requirement and filter providers appropriately */
    /* ep_type:  reliable datagram operation                                    */
    /* caps:     Capabilities required from the provider.  The bits specified   */
    /*           with buffered receive, cancel, and remote complete implements  */
    /*           MPI semantics.  Tagged is used to support tag matching.        */
    /*           We expect to register all memory up front for use with this    */
    /*           endpoint, so the netmod requires dynamic memory regions        */
    /* ------------------------------------------------------------------------ */
    memset(&hints, 0, sizeof(hints));
    hints.mode = FI_CONTEXT;
    hints.ep_type = FI_EP_RDM;  /* Reliable datagram         */
    hints.caps = FI_TAGGED;     /* Tag matching interface    */
    hints.caps |= FI_BUFFERED_RECV;     /* Buffered receives         */
    hints.caps |= FI_CANCEL;    /* Support cancel            */
    hints.caps |= FI_DYNAMIC_MR;        /* Global dynamic mem region */

    /* ------------------------------------------------------------------------ */
    /* FI_VERSION provides binary backward and forward compatibility support    */
    /* Specify the version of OFI is coded to, the provider will select struct  */
    /* layouts that are compatible with this version.                           */
    /* ------------------------------------------------------------------------ */
    fi_version = FI_VERSION(1, 0);

    /* ------------------------------------------------------------------------ */
    /* fi_getinfo:  returns information about fabric  services for reaching a   */
    /* remote node or service.  this does not necessarily allocate resources.   */
    /* Pass NULL for name/service because we want a list of providers supported */
    /* ------------------------------------------------------------------------ */
    domain_attr_t domain_attr;
    memset(&domain_attr, 0, sizeof(domain_attr));

    tx_attr_t tx_attr;
    memset(&tx_attr, 0, sizeof(tx_attr));

    domain_attr.threading = FI_THREAD_ENDPOINT;
    domain_attr.control_progress = FI_PROGRESS_AUTO;
    domain_attr.data_progress = FI_PROGRESS_AUTO;
    hints.domain_attr = &domain_attr;
    hints.tx_attr = &tx_attr;

    FI_RC(fi_getinfo(fi_version,        /* Interface version requested               */
                     NULL,      /* Optional name or fabric to resolve        */
                     NULL,      /* Service name or port number to request    */
                     0ULL,      /* Flag:  node/service specify local address */
                     &hints,    /* In:  Hints to filter available providers  */
                     &prov_tagged),     /* Out: List of providers that match hints   */
          getinfo);
    MPIU_ERR_CHKANDJUMP4(prov_tagged == NULL, mpi_errno, MPI_ERR_OTHER,
                         "**ofi_getinfo", "**ofi_getinfo %s %d %s %s",
                         __SHORT_FILE__, __LINE__, FCNAME, "No tag matching provider found");
    /* ------------------------------------------------------------------------ */
    /* Open fabric                                                              */
    /* The getinfo struct returns a fabric attribute struct that can be used to */
    /* instantiate the virtual or physical network.  This opens a "fabric       */
    /* provider".   We choose the first available fabric, but getinfo           */
    /* returns a list.  see man fi_fabric for details                           */
    /* ------------------------------------------------------------------------ */
    dump_and_choose_providers(prov_tagged, &prov_use);
    FI_RC(fi_fabric(prov_use->fabric_attr,      /* In:   Fabric attributes */
                    &gl_data.fabric,    /* Out:  Fabric descriptor */
                    NULL), openfabric); /* Context: fabric events  */

    /* ------------------------------------------------------------------------ */
    /* Create the access domain, which is the physical or virtual network or    */
    /* hardware port/collection of ports.  Returns a domain object that can be  */
    /* used to create endpoints.  See man fi_domain for details.                */
    /* Refine get_info filter for additional capabilities                       */
    /* threading:  Disable locking, MPICH handles locking model                 */
    /* control_progress:  enable async progress                                 */
    /* op_flags:  Specifies default operation to set on all communication.      */
    /*            In this case, we want remote completion to be set by default  */
    /* ------------------------------------------------------------------------ */
    FI_RC(fi_domain(gl_data.fabric,     /* In:  Fabric object             */
                    prov_use,   /* In:  default domain attributes */
                    &gl_data.domain,    /* Out: domain object             */
                    NULL), opendomain); /* Context: Domain events         */

    /* ------------------------------------------------------------------------ */
    /* Create a transport level communication endpoint.  To use the endpoint,   */
    /* it must be bound to completion counters or event queues and enabled,     */
    /* and the resources consumed by it, such as address vectors, counters,     */
    /* completion queues, etc.                                                  */
    /* see man fi_endpoint for more details                                     */
    /* ------------------------------------------------------------------------ */
    FI_RC(fi_endpoint(gl_data.domain,   /* In: Domain Object        */
                      prov_use, /* In: Configuration object */
                      &gl_data.endpoint,        /* Out: Endpoint Object     */
                      NULL), openep);   /* Context: endpoint events */

    /* ------------------------------------------------------------------------ */
    /* Create the objects that will be bound to the endpoint.                   */
    /* The objects include:                                                     */
    /*     * completion queue for events                                        */
    /*     * address vector of other endpoint addresses                         */
    /*     * dynamic memory-spanning memory region                              */
    /* Other objects could be created (for example), but are unused in netmod   */
    /*     * counters for incoming writes                                       */
    /*     * completion counters for put and get                                */
    /* ------------------------------------------------------------------------ */
    FI_RC(fi_mr_reg(gl_data.domain,     /* In:  Domain Object              */
                    0,  /* In:  Lower memory address       */
                    UINTPTR_MAX,        /* In:  Upper memory address       */
                    FI_SEND | FI_RECV,  /* In:  Expose MR for read/write   */
                    0ULL,       /* In:  base MR offset             */
                    0ULL,       /* In:  requested key              */
                    0ULL,       /* In:  No flags                   */
                    &gl_data.mr,        /* Out: memregion object           */
                    NULL), mr_reg);     /* Context: memregion events       */

    memset(&cq_attr, 0, sizeof(cq_attr));
    cq_attr.format = FI_CQ_FORMAT_TAGGED;
    FI_RC(fi_cq_open(gl_data.domain,    /* In:  Domain Object         */
                     &cq_attr,  /* In:  Configuration object  */
                     &gl_data.cq,       /* Out: CQ Object             */
                     NULL), opencq);    /* Context: CQ events         */

    memset(&av_attr, 0, sizeof(av_attr));
    av_attr.type = FI_AV_MAP;   /* Mapped addressing mode     */
    FI_RC(fi_av_open(gl_data.domain,    /* In:  Domain Object         */
                     &av_attr,  /* In:  Configuration object  */
                     &gl_data.av,       /* Out: AV Object             */
                     NULL), avopen);    /* Context: AV events         */

    /* --------------------------------------------- */
    /* Bind the MR, CQ and AV to the endpoint object */
    /* --------------------------------------------- */
    FI_RC(fi_ep_bind(gl_data.endpoint, (fid_t) gl_data.mr, 0), bind);
    FI_RC(fi_ep_bind(gl_data.endpoint, (fid_t) gl_data.cq, FI_SEND | FI_RECV), bind);
    FI_RC(fi_ep_bind(gl_data.endpoint, (fid_t) gl_data.av, 0), bind);

    /* ------------------------------------- */
    /* Enable the endpoint for communication */
    /* This commits the bind operations      */
    /* ------------------------------------- */
    FI_RC(fi_enable(gl_data.endpoint), ep_enable);

    /* --------------------------- */
    /* Free providers info         */
    /* --------------------------- */
    fi_freeinfo(prov_use);

    /* ---------------------------------------------------- */
    /* Exchange endpoint addresses using scalable database  */
    /* or job launcher, in this case, use PMI interfaces    */
    /* ---------------------------------------------------- */
    gl_data.bound_addrlen = sizeof(gl_data.bound_addr);
    FI_RC(fi_getname((fid_t) gl_data.endpoint, &gl_data.bound_addr,
                     &gl_data.bound_addrlen), getname);

    /* -------------------------------- */
    /* Get our business card            */
    /* -------------------------------- */
    my_bc = *bc_val_p;
    MPI_RC(MPID_nem_ofi_get_business_card(pg_rank, bc_val_p, val_max_sz_p));

    /* -------------------------------- */
    /* Publish the business card        */
    /* to the KVS                       */
    /* -------------------------------- */
    PMI_RC(PMI_KVS_Get_my_name(kvsname, OFI_KVSAPPSTRLEN), pmi);
    sprintf(key, "OFI-%d", pg_rank);

    PMI_RC(PMI_KVS_Put(kvsname, key, my_bc), pmi);
    PMI_RC(PMI_KVS_Commit(kvsname), pmi);

    /* -------------------------------- */
    /* Set the MPI maximum tag value    */
    /* -------------------------------- */
    MPIR_Process.attrs.tag_ub = (1 << MPID_TAG_SHIFT) - 1;

    /* --------------------------------- */
    /* Wait for all the ranks to publish */
    /* their business card               */
    /* --------------------------------- */
    PMI_Barrier();

    /* --------------------------------- */
    /* Retrieve every rank's address     */
    /* from KVS and store them in local  */
    /* table                             */
    /* --------------------------------- */
    MPIU_CHKLMEM_MALLOC(addrs, char *, pg_p->size * gl_data.bound_addrlen, mpi_errno, "addrs");

    for (i = 0; i < pg_p->size; ++i) {
        sprintf(key, "OFI-%d", i);

        PMI_RC(PMI_KVS_Get(kvsname, key, bc, OFI_KVSAPPSTRLEN), pmi);
        ret = MPIU_Str_get_binary_arg(bc, "OFI",
                                      (char *) &addrs[i * gl_data.bound_addrlen],
                                      gl_data.bound_addrlen, &len);
        MPIU_ERR_CHKANDJUMP((ret != MPIU_STR_SUCCESS && ret != MPIU_STR_NOMEM) ||
                            (size_t) len != gl_data.bound_addrlen,
                            mpi_errno, MPI_ERR_OTHER, "**badbusinesscard");
    }

    /* ---------------------------------------------------- */
    /* Map the addresses into an address vector             */
    /* The addressing mode is "map", so we must provide     */
    /* storage to store the per destination addresses       */
    /* ---------------------------------------------------- */
    fi_addrs = MPIU_Malloc(pg_p->size * sizeof(fi_addr_t));
    FI_RC(fi_av_insert(gl_data.av, addrs, pg_p->size, fi_addrs, 0ULL, NULL), avmap);

    /* ---------------------------------------------------- */
    /* Insert the ANY_SRC address                           */
    /* ---------------------------------------------------- */

    gl_data.any_addr = FI_ADDR_UNSPEC;

    /* --------------------------------- */
    /* Store the direct addresses in     */
    /* the ranks' respective VCs         */
    /* --------------------------------- */
    for (i = 0; i < pg_p->size; ++i) {
        MPIDI_PG_Get_vc(pg_p, i, &vc);
        VC_OFI(vc)->direct_addr = fi_addrs[i];
        VC_OFI(vc)->ready = 1;
    }

    /* --------------------------------------------- */
    /* Initialize the connection management routines */
    /* This completes any function handlers and      */
    /* global data structures, and posts any         */
    /* persistent communication requests that are    */
    /* required, like connection management and      */
    /* startcontig messages                          */
    /* --------------------------------------------- */
    MPI_RC(MPID_nem_ofi_cm_init(pg_p, pg_rank));
  fn_exit:
    if (fi_addrs)
        MPIU_Free(fi_addrs);
    MPIU_CHKLMEM_FREEALL();
    END_FUNC(FCNAME);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_finalize)
int MPID_nem_ofi_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret = 0;
    BEGIN_FUNC(FCNAME);

    /* --------------------------------------------- */
    /* Syncronization                                */
    /* Barrier across all ranks in this world        */
    /* --------------------------------------------- */
    MPIR_Barrier_impl(MPIR_Process.comm_world, &ret);

    /* --------------------------------------------- */
    /* Finalize connection management routines       */
    /* Cancels any persistent/global requests and    */
    /* frees any resources from cm_init()            */
    /* --------------------------------------------- */
    MPI_RC(MPID_nem_ofi_cm_finalize());

    FI_RC(fi_close((fid_t) gl_data.mr), mrclose);
    FI_RC(fi_close((fid_t) gl_data.av), avclose);
    FI_RC(fi_close((fid_t) gl_data.endpoint), epclose);
    FI_RC(fi_close((fid_t) gl_data.cq), cqclose);
    FI_RC(fi_close((fid_t) gl_data.domain), domainclose);
    FI_RC(fi_close((fid_t) gl_data.fabric), fabricclose);
    END_FUNC_RC(FCNAME);
}

static inline int compile_time_checking()
{
    OFI_COMPILE_TIME_ASSERT(sizeof(MPID_nem_ofi_vc_t) <= MPID_NEM_VC_NETMOD_AREA_LEN);
    OFI_COMPILE_TIME_ASSERT(sizeof(MPID_nem_ofi_req_t) <= MPID_NEM_REQ_NETMOD_AREA_LEN);
    OFI_COMPILE_TIME_ASSERT(sizeof(iovec_t) == sizeof(MPID_IOV));
    MPIU_Assert(((void *) &(((iovec_t *) 0)->iov_base)) ==
                ((void *) &(((MPID_IOV *) 0)->MPID_IOV_BUF)));
    MPIU_Assert(((void *) &(((iovec_t *) 0)->iov_len)) ==
                ((void *) &(((MPID_IOV *) 0)->MPID_IOV_LEN)));
    MPIU_Assert(sizeof(((iovec_t *) 0)->iov_len) == sizeof(((MPID_IOV *) 0)->MPID_IOV_LEN));

    /* ------------------------------------------------------------------------ */
    /* Generate the MPICH catalog files                                         */
    /* The high level mpich build scripts inspect MPIU_ERR_ macros to generate  */
    /* the message catalog.  However, this netmod buries the messages under the */
    /* FI_RC macros, so the catalog doesn't get generated.  The build system    */
    /* likely needs a MPIU_ERR_REGISTER macro                                   */
    /* ------------------------------------------------------------------------ */
#if 0
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_avmap", "**ofi_avmap %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_tsend", "**ofi_tsend %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_trecv", "**ofi_trecv %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_getinfo", "**ofi_getinfo %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_openep", "**ofi_openep %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_openfabric", "**ofi_openfabric %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_opendomain", "**ofi_opendomain %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_opencq", "**ofi_opencq %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_avopen", "**ofi_avopen %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_bind", "**ofi_bind %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_ep_enable", "**ofi_ep_enable %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_getname", "**ofi_getname %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_avclose", "**ofi_avclose %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_epclose", "**ofi_epclose %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_cqclose", "**ofi_cqclose %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_fabricclose", "**ofi_fabricclose %s %d %s %s", a, b, a,
                  a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_domainclose", "**ofi_domainclose %s %d %s %s", a, b, a,
                  a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_tsearch", "**ofi_tsearch %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_poll", "**ofi_poll %s %d %s %s", a, b, a, a);
    MPIU_ERR_SET2(e, MPI_ERR_OTHER, "**ofi_cancel", "**ofi_cancel %s %d %s %s", a, b, a, a);
#endif
    return 0;
}


/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_DUMP_PROVIDERS
      category    : DEVELOPER
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_MPIDEV_DETAIL
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, dump provider information at init

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/
static inline int dump_and_choose_providers(info_t * prov, info_t ** prov_use)
{
    info_t *p = prov;
    int i = 0;
    *prov_use = prov;
    if (MPIR_CVAR_DUMP_PROVIDERS) {
        fprintf(stdout, "Dumping Providers(first=%p):\n", prov);
        while (p) {
            fprintf(stdout, " ********** Provider %d (%p) *********\n", i++, p);
            fprintf(stdout, "%-18s: %-#20" PRIx64 "\n", "caps", p->caps);
            fprintf(stdout, "%-18s: %-#20" PRIx64 "\n", "mode", p->mode);
            fprintf(stdout, "%-18s: %-#20" PRIx32 "\n", "ep_type", p->ep_type);
            fprintf(stdout, "%-18s: %-#20" PRIx32 "\n", "addr_format", p->addr_format);
            fprintf(stdout, "%-18s: %-20lu\n", "src_addrlen", p->src_addrlen);
            fprintf(stdout, "%-18s: %-20lu\n", "dest_addrlen", p->dest_addrlen);
            fprintf(stdout, "%-18s: %-20p\n", "src_addr", p->src_addr);
            fprintf(stdout, "%-18s: %-20p\n", "dest_addr", p->dest_addr);
            fprintf(stdout, "%-18s: %-20p\n", "connreq", p->connreq);
            fprintf(stdout, "%-18s: %-20p\n", "tx_attr", p->tx_attr);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".caps", p->tx_attr->caps);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".mode", p->tx_attr->mode);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".op_flags", p->tx_attr->op_flags);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".msg_order", p->tx_attr->msg_order);
            fprintf(stdout, "       %-18s: %-20lu\n", ".inject_size", p->tx_attr->inject_size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".size", p->tx_attr->size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".iov_limit", p->tx_attr->iov_limit);
            fprintf(stdout, "%-18s: %-20p\n", "rx_attr", p->rx_attr);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".caps", p->rx_attr->caps);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".mode", p->rx_attr->mode);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".op_flags", p->rx_attr->op_flags);
            fprintf(stdout, "       %-18s: %-#20" PRIx64 "\n", ".msg_order", p->rx_attr->msg_order);
            fprintf(stdout, "       %-18s: %-20lu\n", ".total_buffered_recv",
                    p->rx_attr->total_buffered_recv);
            fprintf(stdout, "       %-18s: %-20lu\n", ".size", p->rx_attr->size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".iov_limit", p->rx_attr->iov_limit);
            fprintf(stdout, "%-18s: %-20p\n", "ep_attr", p->ep_attr);
            fprintf(stdout, "       %-18s: %-#20" PRIx32 "\n", ".protocol", p->ep_attr->protocol);
            fprintf(stdout, "       %-18s: %-20lu\n", ".max_msg_size", p->ep_attr->max_msg_size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".inject_size", p->ep_attr->inject_size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".total_buffered_recv",
                    p->ep_attr->total_buffered_recv);
            fprintf(stdout, "       %-18s: %-20lu\n", ".max_order_raw_size",
                    p->ep_attr->max_order_raw_size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".max_order_war_size",
                    p->ep_attr->max_order_war_size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".max_order_waw_size",
                    p->ep_attr->max_order_waw_size);
            fprintf(stdout, "       %-18s: %-20lu\n", ".mem_tag_format",
                    p->ep_attr->mem_tag_format);
            fprintf(stdout, "       %-18s: %-20lu\n", ".msg_order", p->ep_attr->msg_order);
            fprintf(stdout, "       %-18s: %-20lu\n", ".tx_ctx_cnt", p->ep_attr->tx_ctx_cnt);
            fprintf(stdout, "       %-18s: %-20lu\n", ".rx_ctx_cnt", p->ep_attr->rx_ctx_cnt);
            fprintf(stdout, "%-18s: %-20p\n", "domain_attr", p->domain_attr);
            fprintf(stdout, "           %-18s: %-20s\n", ".name", p->domain_attr->name);
            fprintf(stdout, "           %-18s: %-#20" PRIx32 "\n", ".threading",
                    p->domain_attr->threading);
            fprintf(stdout, "           %-18s: %-#20" PRIx32 "\n", ".control_progress",
                    p->domain_attr->control_progress);
            fprintf(stdout, "           %-18s: %-#20" PRIx32 "\n", ".data_progress",
                    p->domain_attr->data_progress);
            fprintf(stdout, "           %-18s: %-20lu\n", ".mr_key_size",
                    p->domain_attr->mr_key_size);
            fprintf(stdout, "           %-18s: %-20lu\n", ".cq_data_size",
                    p->domain_attr->cq_data_size);
            fprintf(stdout, "           %-18s: %-20lu\n", ".ep_cnt", p->domain_attr->ep_cnt);
            fprintf(stdout, "           %-18s: %-20lu\n", ".tx_ctx_cnt",
                    p->domain_attr->tx_ctx_cnt);
            fprintf(stdout, "           %-18s: %-20lu\n", ".rx_ctx_cnt",
                    p->domain_attr->rx_ctx_cnt);
            fprintf(stdout, "           %-18s: %-20lu\n", ".max_ep_tx_ctx",
                    p->domain_attr->max_ep_tx_ctx);
            fprintf(stdout, "           %-18s: %-20lu\n", ".max_ep_rx_ctx",
                    p->domain_attr->max_ep_rx_ctx);
            fprintf(stdout, "%-18s: %-20p\n", "fabric_attr", p->fabric_attr);
            fprintf(stdout, "           %-18s: %-20s\n", ".name", p->fabric_attr->name);
            fprintf(stdout, "           %-18s: %-20s\n", ".prov_name", p->fabric_attr->prov_name);
            fprintf(stdout, "           %-18s: %-#20" PRIx32 "\n", ".prov_version",
                    p->fabric_attr->prov_version);
            p = p->next;
        }
    }
    return i;
}
