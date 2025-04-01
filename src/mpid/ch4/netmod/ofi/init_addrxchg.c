/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"
#include "mpidu_bc.h"

/* Perform address exchange and enable netmod communication over the comm. */

/* NOTE: we assume the intranode shm communication is already enabled.
 * NOTE: the root av no longer require fixed insertion order. See comments in ofi_vci.c.
 */
int MPIDI_OFI_comm_addr_exchange(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    /* only comm_world for now
     * TODO: Use an attribute to mark whether this is a comm from
     *       MPI_Comm_create_from_group and to be checked in NM_mpi_comm_pre_hook.
     */
    MPIR_Assert(comm == MPIR_Process.comm_world);

    MPIR_Assert(comm->attr & MPIR_COMM_ATTR__HIERARCHY);

    /* First, each get its own name */
    MPIDI_OFI_global.addrnamelen = FI_NAME_MAX;
    MPIDI_OFI_CALL(fi_getname
                   ((fid_t) MPIDI_OFI_global.ctx[0].ep, MPIDI_OFI_global.addrname[0],
                    &MPIDI_OFI_global.addrnamelen), getname);

    char *addrname = MPIDI_OFI_global.addrname[0];
    int addrnamelen = MPIDI_OFI_global.addrnamelen;

    int local_rank = comm->local_rank;
    int external_size = comm->num_external;

    if (external_size == 1) {
        /* skip root addrexch if we are the only node */
        goto all_addrexch;
    }

    /* PMI allgather over node roots and av_insert to activate node_roots_comm */
    if (local_rank == 0) {
        char *roots_names;
        MPIR_CHKLMEM_MALLOC(roots_names, external_size * addrnamelen);

        MPIR_PMI_DOMAIN domain = MPIR_PMI_DOMAIN_NODE_ROOTS;
        mpi_errno = MPIR_pmi_allgather(addrname, addrnamelen, roots_names, addrnamelen, domain);
        MPIR_ERR_CHECK(mpi_errno);

        /* insert av and activate node_roots_comm */
        MPIR_Comm *node_roots_comm = MPIR_Comm_get_node_roots_comm(comm);
        for (int i = 0; i < external_size; i++) {
            MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(node_roots_comm, i);
            MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(node_roots_comm, i);
            if (MPIDI_OFI_AV_IS_UNSET(av, lpid)) {
                MPIDI_OFI_CALL(fi_av_insert
                               (MPIDI_OFI_global.ctx[0].av, roots_names + i * addrnamelen, 1,
                                &MPIDI_OFI_AV_ADDR_ROOT(av), 0ULL, NULL), avmap);
                if (MPIDI_OFI_AV_ADDR_ROOT(av) == 0) {
                    MPIDI_OFI_global.lpid0 = lpid;
                }
            }
        }
    } else {
        /* just for the PMI_Barrier */
        MPIR_PMI_DOMAIN domain = MPIR_PMI_DOMAIN_NODE_ROOTS;
        mpi_errno = MPIR_pmi_allgather(addrname, addrnamelen, NULL, addrnamelen, domain);
        MPIR_ERR_CHECK(mpi_errno);
    }

  all_addrexch:
    if (external_size == comm->local_size) {
        /* if no local, we are done. */
        goto fn_exit;
    }

    /* -- rest of the addr exchange over node_code and node_roots_comm -- */
    /* since the orders will be rearranged by nodes, let's echange rank along with name */
    struct rankname {
        int rank;
        char name[];
    };
    int rankname_len = sizeof(int) + addrnamelen;

    struct rankname *my_rankname, *all_ranknames;
    MPIR_CHKLMEM_MALLOC(my_rankname, rankname_len);
    MPIR_CHKLMEM_MALLOC(all_ranknames, comm->local_size * rankname_len);

    my_rankname->rank = comm->rank;
    memcpy(my_rankname->name, addrname, addrnamelen);

    /* Use an smp algorithm explicitly that only require a working node_comm and node_roots_comm.
     * The node_comm is activated by MPIDI_SHM_mpi_comm_commit_pre_hook. The node_roots_comm
     * is activated by the earlier address exchange using PMI.
     */
    mpi_errno = MPIR_Allgather_intra_smp_no_order(my_rankname, rankname_len, MPIR_BYTE_INTERNAL,
                                                  all_ranknames, rankname_len, MPIR_BYTE_INTERNAL,
                                                  comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* av insert, skipping over existing entries */
    for (int i = 0; i < comm->local_size; i++) {
        struct rankname *p = (struct rankname *) ((char *) all_ranknames + i * rankname_len);
        MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, p->rank);
        MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(comm, p->rank);
        if (MPIDI_OFI_AV_IS_UNSET(av, lpid)) {
            MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[0].av, p->name, 1,
                                        &MPIDI_OFI_AV_ADDR_ROOT(av), 0ULL, NULL), avmap);
            if (MPIDI_OFI_AV_ADDR_ROOT(av) == 0) {
                MPIDI_OFI_global.lpid0 = lpid;
            }
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
