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
 * Each nic-vni is an endpoint with a unique address, and inside libfabric maintains
 * one av table. Thus to fully store the address mapping, we'll need a multi-dim table as
 *     av_table[src_nic][src_vni][dest_rank][dest_nic][dest_vni]
 * Note, this table is for illustration, and different from MPIDI_OFI_addr_t.
 *
 * However, if we insert the address carefully, we can manage to make the av table inside
 * each endpoint *identical*. Then, we can omit the dimension of [src_nic][src_vni].
 *
 * To achieve that, we use the following 3-step process (described with above illustrative av_table).
 *
 * Step 1. insert and store       av_table[ 0 ][ 0 ][rank][ 0 ][ 0 ]
 *
 * Step 2. insert and store       av_table[ 0 ][ 0 ][rank][nic][vni]
 *
 * Step 3. insert (but not store) av_table[nic][vni][rank][nic][vni]
 *
 * The step 1 is done in addr_exchange_root_vni. Step 2 and 3 are done in addr_exchange_all_vnis.
 * Step 3 populates av tables inside libfabric for all non-zero endpoints, but they should be
 * identical to the table in root endpoint, thus no need to store them in mpich. Thus the table is
 * reduced to
 *      av_table[rank] -> dest[nic][vni]
 *
 * With single-nic and single-vni, only step 1 is needed.
 *
 * We do step 1 during world-init, and step 2 & 3 during post-init. The separation
 * isolates multi-nic/vni complications from bootstrapping phase.
 */

/* with MPIDI_OFI_ENABLE_AV_TABLE, we potentially can omit storing av tables.
 * The following routines ensures we can do that. It is static now, but we can
 * easily export to global when we need to.
 */
ATTRIBUTE((unused))
static int get_av_table_index(int rank, int nic, int vni)
{
    if (nic == 0 && vni == 0) {
        if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
            /* check node roots */
            for (int i = 0; i < MPIR_Process.num_nodes; i++) {
                if (MPIR_Process.node_root_map[i] == rank) {
                    return i;
                }
            }
            /* must be non-node-root */
            int num_later_nodes = MPIR_Process.num_nodes - (MPIR_Process.node_map[rank] + 1);
            return rank + num_later_nodes;
        } else {
            return rank;
        }
    } else {
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
        int num_vnis = MPIDI_OFI_global.num_vnis;
#else
        /* with scalable endpoint as context, all vnis share the same address. */
        int num_vnis = 1;
#endif
        int num_nics = MPIDI_OFI_global.num_nics;
        int num_later_ranks = MPIR_Process.size - (rank + 1);
        return rank * num_nics * num_vnis + nic * num_vnis + vni + num_later_ranks;
    }
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

        for (int i = 0; i < num_nodes; i++) {
            MPIR_Assert(mapped_table[i] != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV(&MPIDIU_get_av(0, node_roots[i])).dest[0][0] = mapped_table[i];
        }
        MPL_free(mapped_table);
        /* Then, allgather all address names using init_comm */
        MPIDU_bc_allgather(init_comm, MPIDI_OFI_global.addrname, MPIDI_OFI_global.addrnamelen,
                           TRUE, &table, &rank_map, &recv_bc_len);

        /* Insert the rest of the addresses */
        for (int i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {
                fi_addr_t addr;
                char *addrname = (char *) table + recv_bc_len * rank_map[i];
                MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[0].av,
                                            addrname, 1, &addr, 0ULL, NULL), avmap);
                MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest[0][0] = addr;
            }
        }
        MPIDU_bc_table_destroy();
    } else {
        /* not "ROOTS_ONLY", we already have everyone's address name, insert all of them */
        fi_addr_t *mapped_table;
        mapped_table = (fi_addr_t *) MPL_malloc(size * sizeof(fi_addr_t), MPL_MEM_ADDRESS);
        MPIDI_OFI_CALL(fi_av_insert
                       (MPIDI_OFI_global.ctx[0].av, table, size, mapped_table, 0ULL, NULL), avmap);

        for (int i = 0; i < size; i++) {
            MPIR_Assert(mapped_table[i] != FI_ADDR_NOTAVAIL);
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest[0][0] = mapped_table[i];
        }
        MPL_free(mapped_table);
        MPIDU_bc_table_destroy();
    }

  fn_exit:
    if (init_comm) {
        MPIDI_destroy_init_comm(&init_comm);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Step 2 & 3: exchange non-root contexts */

/* Macros to reduce clutter, so we can focus on the ordering logics.
 * Note: they are not perfectly wrapped, but tolerable since only used here. */
#define GET_AV_AND_ADDRNAMES(rank) \
    MPIDI_OFI_addr_t *av ATTRIBUTE((unused)) = &MPIDI_OFI_AV(&MPIDIU_get_av(0, rank)); \
    char *r_names = all_names + rank * num_vnis * num_nics * name_len;

#define DO_AV_INSERT(ctx_idx, nic, vni) \
    fi_addr_t addr; \
    MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[ctx_idx].av, \
                                r_names + (vni * num_nics + nic) * name_len, 1, \
                                &addr, 0ULL, NULL), avmap);

#define SKIP_ROOT(nic, vni) \
    if (nic == 0 && vni == 0) { \
        continue; \
    }

int MPIDI_OFI_addr_exchange_all_ctx(void)
{
    int mpi_errno = MPI_SUCCESS;

    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    int num_vnis = MPIDI_OFI_global.num_vnis;
    int num_nics = MPIDI_OFI_global.num_nics;
    MPIR_CHKLMEM_DECL(2);

#ifndef MPIDI_OFI_VNI_USE_DOMAIN
    /* with scalable endpoint as context, all vnis share the same address. For the
     * purpose of address exchange, we hack it as having single vni.
     */
    num_vnis = 1;
#endif

    if (num_nics * num_vnis == 1) {
        /* root address exchange already done. */
        goto fn_check;
    }

    int *is_node_roots = NULL;
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        MPIR_CHKLMEM_MALLOC(is_node_roots, int *, size * sizeof(int),
                            mpi_errno, "is_node_roots", MPL_MEM_ADDRESS);
        for (int r = 0; r < size; r++) {
            is_node_roots[r] = 0;
        }
        for (int i = 0; i < MPIR_Process.num_nodes; i++) {
            is_node_roots[MPIR_Process.node_root_map[i]] = 1;
        }
    }

    /* libfabric uses uniform name_len within a single provider */
    int name_len = MPIDI_OFI_global.addrnamelen;
    int my_len = num_vnis * num_nics * name_len;
    char *all_names;
    MPIR_CHKLMEM_MALLOC(all_names, char *, size * my_len, mpi_errno, "all_names", MPL_MEM_ADDRESS);
    char *my_names = all_names + rank * my_len;

    /* put in my addrnames */
    for (int nic = 0; nic < num_nics; nic++) {
        for (int vni = 0; vni < num_vnis; vni++) {
            size_t actual_name_len = name_len;
            char *vni_addrname = my_names + (vni * num_nics + nic) * name_len;
            int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);
            MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_OFI_global.ctx[ctx_idx].ep, vni_addrname,
                                      &actual_name_len), getname);
            MPIR_Assert(actual_name_len == name_len);
        }
    }
    /* Allgather */
    MPIR_Comm *comm = MPIR_Process.comm_world;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    mpi_errno = MPIR_Allgather_fallback(MPI_IN_PLACE, 0, MPI_BYTE,
                                        all_names, my_len, MPI_BYTE, comm, &errflag);

    /* Step 2: insert and store non-root nic/vni on the root context */
    int root_ctx_idx = MPIDI_OFI_get_ctx_index(NULL, 0, 0);
    for (int r = 0; r < size; r++) {
        GET_AV_AND_ADDRNAMES(r);
        for (int nic = 0; nic < num_nics; nic++) {
            for (int vni = 0; vni < num_vnis; vni++) {
                SKIP_ROOT(nic, vni);
                DO_AV_INSERT(root_ctx_idx, nic, vni);
                av->dest[nic][vni] = addr;
            }
        }
    }

    /* Step 3: insert all nic/vni on non-root context, following exact order as step 1 and 2 */
    for (int nic_local = 0; nic_local < num_nics; nic_local++) {
        for (int vni_local = 0; vni_local < num_vnis; vni_local++) {
            SKIP_ROOT(nic_local, vni_local);
            int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni_local, nic_local);

            /* -- same order as step 1 -- */
            if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
                /* node roots */
                for (int r = 0; r < size; r++) {
                    if (is_node_roots[r]) {
                        GET_AV_AND_ADDRNAMES(r);
                        DO_AV_INSERT(ctx_idx, 0, 0);
                        MPIR_Assert(av->dest[0][0] == addr);
                    }
                }
                /* non-node-root */
                for (int r = 0; r < size; r++) {
                    if (!is_node_roots[r]) {
                        GET_AV_AND_ADDRNAMES(r);
                        DO_AV_INSERT(ctx_idx, 0, 0);
                        MPIR_Assert(av->dest[0][0] == addr);
                    }
                }
            } else {
                /* !MPIR_CVAR_CH4_ROOTS_ONLY_PMI */
                for (int r = 0; r < size; r++) {
                    GET_AV_AND_ADDRNAMES(r);
                    DO_AV_INSERT(ctx_idx, 0, 0);
                    MPIR_Assert(av->dest[0][0] == addr);
                }
            }

            /* -- same order as step 2 -- */
            for (int r = 0; r < size; r++) {
                GET_AV_AND_ADDRNAMES(r);
                for (int nic = 0; nic < num_nics; nic++) {
                    for (int vni = 0; vni < num_vnis; vni++) {
                        SKIP_ROOT(nic, vni);
                        DO_AV_INSERT(ctx_idx, nic, vni);
                        MPIR_Assert(av->dest[nic][vni] == addr);
                    }
                }
            }
        }
    }
    mpi_errno = MPIR_Barrier_fallback(comm, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_check:
    if (MPIDI_OFI_ENABLE_AV_TABLE) {
        for (int r = 0; r < size; r++) {
            MPIDI_OFI_addr_t *av ATTRIBUTE((unused)) = &MPIDI_OFI_AV(&MPIDIU_get_av(0, r));
            for (int nic = 0; nic < num_nics; nic++) {
                for (int vni = 0; vni < num_vnis; vni++) {
                    MPIR_Assert(av->dest[nic][vni] == get_av_table_index(r, nic, vni));
                }
            }
        }
    }
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
