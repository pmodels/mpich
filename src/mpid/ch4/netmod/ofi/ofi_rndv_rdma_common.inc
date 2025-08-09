/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

struct rdma_info {
    MPI_Aint data_sz;
    uint64_t base;              /* 0 unless MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS is true */
    int num_nics;               /* redundant since we assume sender/receiver agree on num_nics */
    uint64_t rkeys[];
};

static MPI_Aint get_chunks_per_nic(MPI_Aint data_sz, int num_nics);

static int prepare_rdma_info(const void *buf, MPI_Datatype datatype, MPI_Aint data_sz, int vci,
                             int access, struct fid_mr **mrs, struct rdma_info *hdr)
{
    int mpi_errno = MPI_SUCCESS;
    const void *data;
    MPI_Aint true_extent, true_lb;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    data = MPIR_get_contig_ptr(buf, true_lb);

    int num_nics = MPIDI_OFI_global.num_nics;
    uint64_t rkeys[MPIDI_OFI_MAX_NICS];

    /* prepare mr and rkey */
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* Set up a memory region for the lmt data transfer */
        for (int i = 0; i < num_nics; i++) {
            rkeys[i] = MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
            MPIR_ERR_CHKANDJUMP(rkeys[i] == MPIDI_OFI_INVALID_MR_KEY, mpi_errno,
                                MPI_ERR_OTHER, "**ofid_mr_key");
        }
    } else {
        /* zero them to avoid warnings */
        for (int i = 0; i < num_nics; i++) {
            rkeys[i] = 0;
        }
    }
    MPI_Aint chunks_per_nic = get_chunks_per_nic(data_sz, num_nics);
    MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
    MPI_Aint sz_per_nic = chunks_per_nic * chunk_sz;
    for (int i = 0; i < num_nics; i++) {
        MPIDI_OFI_context_t *ctx = &MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(vci, i)];
        /* note: fi_mr_reg is expensive, distribute over num_nics */
        void *nic_data = (char *) data + i * sz_per_nic;
        MPI_Aint sz = (i != num_nics - 1) ? sz_per_nic : (data_sz - i * sz_per_nic);
        MPIDI_OFI_CALL(fi_mr_reg(ctx->domain, nic_data, sz, access, 0ULL,
                                 rkeys[i], 0ULL, &mrs[i], NULL), mr_reg);
        mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], mrs[i], ctx->ep,
                                      NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }
    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        for (int i = 0; i < num_nics; i++) {
            rkeys[i] = fi_mr_key(mrs[i]);
        }
    }

    /* prepare rdma_info */
    hdr->data_sz = data_sz;
    hdr->base = (uintptr_t) data;
    for (int i = 0; i < num_nics; i++) {
        hdr->rkeys[i] = rkeys[i];
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* utility routine for calculating chunks */
/* Each nic is assigned with chunks_per_nic chunks. The last nic may have less chunks */

static MPI_Aint get_chunks_per_nic(MPI_Aint data_sz, int num_nics)
{
    MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
    MPI_Aint num_chunks = data_sz / chunk_sz;
    if (chunk_sz * num_chunks < data_sz) {
        num_chunks++;
    }

    if (num_nics == 1) {
        return num_chunks;
    } else {
        MPI_Aint chunks_per_nic = num_chunks / num_nics;
        if (chunks_per_nic * num_nics < num_chunks) {
            chunks_per_nic++;
        }
        return chunks_per_nic;
    }
}

static void get_chunk_offsets(MPI_Aint chunk_index, int num_nics, MPI_Aint chunks_per_nic,
                              MPI_Aint data_sz, MPI_Aint * total_offset_out, int *nic_out,
                              MPI_Aint * nic_offset_out, MPI_Aint * chunk_sz_out)
{
    MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
    if (num_nics == 1) {
        *nic_out = 0;
        *nic_offset_out = *total_offset_out = chunk_index * chunk_sz;
    } else {
        int nic = chunk_index % num_nics;
        MPI_Aint nic_chunk_index = chunk_index / chunks_per_nic;
        *total_offset_out = (nic * chunks_per_nic + nic_chunk_index) * chunk_sz;
        *nic_offset_out = nic_chunk_index * chunk_sz;
    }
    if (*total_offset_out + chunk_sz < data_sz) {
        *chunk_sz_out = chunk_sz;
    } else {
        /* incomplete chunks */
        *chunk_sz_out = MPL_MAX(0, data_sz - *total_offset_out);
    }
}
