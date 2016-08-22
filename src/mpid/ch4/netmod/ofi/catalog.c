/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
static inline void MPIDI_OFI_unused_gen_catalog()
{
#if 0
    char *a;
    int b, e;
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_pmi", "**ofid_pmi %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_addrinfo", "**ofid_addrinfo %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_opendomain", "**ofid_opendomain %s %d %s %s", a, b, a,
                  a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_bind", "**ofid_bind %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_fabric", "**ofid_fabric %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_opencq", "**ofid_opencq %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_openct", "**ofid_openct %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_bind", "**ofid_bind %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_ep_enable", "**ofid_ep_enable %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_ep", "**ofid_ep %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_avopen", "**ofid_avopen %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_getname", "**ofid_getname %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_avmap", "**ofid_avmap %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_avlookup", "**ofid_avlookup %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_avsync", "**ofid_avsync %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_epclose", "**ofid_epclose %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_cqclose", "**ofid_cqclose %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_epsync", "**ofid_epsync %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_alias", "**ofid_alias %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_getopt", "**ofid_getopt %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_setopt", "**ofid_setopt %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_domainclose", "**ofid_domainclose %s %d %s %s", a, b, a,
                  a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_avclose", "**ofid_avclose %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_tsend", "**ofid_tsend %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_tinject", "**ofid_tinject %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_tsendsync", "**ofid_tsendsync %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_trecv", "**ofid_trecv %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_trecvsync", "**ofid_trecvsync %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_poll", "**ofid_poll %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_peek", "**ofid_peek %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_send", "**ofid_send %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_inject", "**ofid_inject %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_rdma_write", "**ofid_rdma_write %s %d %s %s", a, b, a,
                  a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_rdma_inject_write",
                  "**ofid_rdma_inject_write %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_rdma_atomicto", "**ofid_rdma_atomicto %s %d %s %s", a,
                  b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_rdma_cswap", "**ofid_rdma_cswap %s %d %s %s", a, b, a,
                  a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_rdma_readfrom", "**ofid_rdma_readfrom %s %d %s %s", a,
                  b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_rdma_readfrom", "**ofid_rdma_readfrom %s %d %s %s", a,
                  b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_mr_reg", "**ofid_mr_reg %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_mr_unreg", "**ofid_mr_unreg %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_prepost", "**ofid_prepost %s %d %s %s", a, b, a, a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_ctrlcancel", "**ofid_ctrlcancel %s %d %s %s", a, b, a,
                  a);
    MPIR_ERR_SET2(e, MPI_ERR_OTHER, "**ofid_cntr_wait", "**ofid_cntr_wait %s %d %s %s", a, b, a, a);

#endif
}
