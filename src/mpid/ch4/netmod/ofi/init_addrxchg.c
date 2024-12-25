/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"
#include "mpidu_bc.h"

/* NOTE on av insertion order:
 *
 * Each nic-vci is an endpoint with a unique address, and inside libfabric maintains
 * one av table. Thus to fully store the address mapping, we'll need a multi-dim table as
 *     av_table[src_nic][src_vci][dest_rank][dest_nic][dest_vci]
 * Note, this table is for illustration, and different from MPIDI_OFI_addr_t.
 *
 * However, if we insert the address carefully, we can manage to make the av table inside
 * each endpoint *identical*. Then, we can omit the dimension of [src_nic][src_vci].
 *
 * To achieve that, we use the following 3-step process (described with above illustrative av_table).
 *
 * Step 1. insert and store       av_table[ 0 ][ 0 ][rank][ 0 ][ 0 ]
 *
 * Step 2. insert and store       av_table[ 0 ][ 0 ][rank][nic][vci]
 *
 * Step 3. insert (but not store) av_table[nic][vci][rank][nic][vci]
 *
 * The step 1 is done in addr_exchange_root_vci. Step 2 and 3 are done in addr_exchange_all_vcis.
 * Step 3 populates av tables inside libfabric for all non-zero endpoints, but they should be
 * identical to the table in root endpoint, thus no need to store them in mpich. Thus the table is
 * reduced to
 *      av_table[rank] -> dest[nic][vci]
 *
 * With single-nic and single-vci, only step 1 is needed.
 *
 * We do step 1 during world-init, and step 2 & 3 during post-init. The separation
 * isolates multi-nic/vci complications from bootstrapping phase.
 */

/* Step 1: exchange root contexts */
int MPIDI_OFI_addr_exchange_root_ctx(void)
{
    int mpi_errno = MPI_SUCCESS;
    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    MPIR_Comm *init_comm = NULL;

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
    mpi_errno = MPIDU_bc_table_create(rank, size, MPIR_Process.node_map,
                                      &MPIDI_OFI_global.addrname, MPIDI_OFI_global.addrnamelen,
                                      TRUE, MPIR_CVAR_CH4_ROOTS_ONLY_PMI, &table, NULL);
    MPIR_ERR_CHECK(mpi_errno);

    /* Third, each fi_av_insert those addresses */
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        /* if "ROOTS_ONLY", we do a two stage bootstrapping ... */
        int num_nodes = MPIR_Process.num_nodes;
        int *node_roots = MPIR_Process.node_root_map;
        int *rank_map, recv_bc_len;

        mpi_errno = MPIDI_create_init_comm(&init_comm);
        MPIR_ERR_CHECK(mpi_errno);

        /* First, insert address of node-roots, init_comm become useful */
        fi_addr_t *mapped_table;
        mapped_table = (fi_addr_t *) MPL_malloc(num_nodes * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
        MPIDI_OFI_CALL(fi_av_insert
                       (MPIDI_OFI_global.ctx[0].av, table, num_nodes, mapped_table, 0ULL, NULL),
                       avmap);

        if (mapped_table[0] == 0) {
            MPIDI_OFI_global.lpid0 = node_roots[0];
        }
        for (int i = 0; i < num_nodes; i++) {
            MPIR_Assert(mapped_table[i] != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV_ADDR_ROOT(MPIDIU_lpid_to_av(node_roots[i])) = mapped_table[i];
        }
        MPL_free(mapped_table);
        /* Then, allgather all address names using init_comm */
        mpi_errno =
            MPIDU_bc_allgather(init_comm, MPIDI_OFI_global.addrname, MPIDI_OFI_global.addrnamelen,
                               TRUE, &table, &rank_map, &recv_bc_len);
        MPIR_ERR_CHECK(mpi_errno);

        /* Insert the rest of the addresses */
        for (int i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {
                fi_addr_t addr;
                char *addrname = (char *) table + recv_bc_len * rank_map[i];
                MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[0].av,
                                            addrname, 1, &addr, 0ULL, NULL), avmap);
                MPIDI_OFI_AV_ADDR_ROOT(MPIDIU_lpid_to_av(i)) = addr;
            }
        }
        mpi_errno = MPIDU_bc_table_destroy();
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* not "ROOTS_ONLY", we already have everyone's address name, insert all of them */
        fi_addr_t *mapped_table;
        mapped_table = (fi_addr_t *) MPL_malloc(size * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
        MPIDI_OFI_CALL(fi_av_insert
                       (MPIDI_OFI_global.ctx[0].av, table, size, mapped_table, 0ULL, NULL), avmap);

        if (mapped_table[0] == 0) {
            MPIDI_OFI_global.lpid0 = 0;
        }
        for (int i = 0; i < size; i++) {
            MPIR_Assert(mapped_table[i] != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV_ADDR_ROOT(MPIDIU_lpid_to_av(i)) = mapped_table[i];
        }
        MPL_free(mapped_table);
        mpi_errno = MPIDU_bc_table_destroy();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    if (init_comm && !mpi_errno) {
        MPIDI_destroy_init_comm(&init_comm);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
