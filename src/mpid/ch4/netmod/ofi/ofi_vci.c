/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

int MPIDI_OFI_vci_init(void)
{
    MPIDI_OFI_global.num_nics = 1;
    MPIDI_OFI_global.num_vcis = 1;
    return MPI_SUCCESS;
}

/* Address exchange within comm and setup multiple vcis */
static int addr_exchange_all_ctx(MPIR_Comm * comm);

int MPIDI_OFI_comm_set_vcis(MPIR_Comm * comm, int num_vcis, int *all_num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    /* set up local vcis */
    int num_vcis_actual;
    mpi_errno = MPIDI_OFI_init_vcis(num_vcis, &num_vcis_actual);
    MPIR_ERR_CHECK(mpi_errno);

    /* gather the number of remote vcis */
    mpi_errno = MPIR_Allgather_impl(&num_vcis_actual, 1, MPIR_INT_INTERNAL,
                                    all_num_vcis, 1, MPIR_INT_INTERNAL, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* Since we allow different process to have different num_vcis, we always need run exchange. */
    mpi_errno = addr_exchange_all_ctx(comm);
    MPIR_ERR_CHECK(mpi_errno);

    for (int vci = 1; vci < MPIDI_OFI_global.num_vcis; vci++) {
        MPIDI_OFI_am_init(vci);
        MPIDI_OFI_am_post_recv(vci, 0);
    }

    if (MPIR_CVAR_DEBUG_SUMMARY && comm->rank == 0) {
        fprintf(stdout, "==== OFI dynamic settings ====\n");
        fprintf(stdout, "num_vcis: %d\n", MPIDI_OFI_global.num_vcis);
        fprintf(stdout, "num_nics: %d\n", MPIDI_OFI_global.num_nics);
        fprintf(stdout, "======================================\n");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDI_OFI_init_vcis: locally create multiple vcis */

static int check_num_nics(void);
static int setup_additional_vcis(void);

int MPIDI_OFI_init_vcis(int num_vcis, int *num_vcis_actual)
{
    int mpi_errno = MPI_SUCCESS;

    /* Multiple vci without using domain require MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS */
#ifndef MPIDI_OFI_VNI_USE_DOMAIN
    MPIR_Assert(num_vcis == 1 || MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
#endif

    MPIDI_OFI_global.num_vcis = num_vcis;

    /* All processes must have the same number of NICs */
    mpi_errno = check_num_nics();
    MPIR_ERR_CHECK(mpi_errno);

    /* may update MPIDI_OFI_global.num_vcis */
    mpi_errno = setup_additional_vcis();
    MPIR_ERR_CHECK(mpi_errno);

    *num_vcis_actual = MPIDI_OFI_global.num_vcis;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int check_num_nics(void)
{
    int mpi_errno = MPI_SUCCESS;

    int num_nics = MPIDI_OFI_global.num_nics;
    int tmp_num_vcis = MPIDI_OFI_global.num_vcis;
    int tmp_num_nics = MPIDI_OFI_global.num_nics;

    /* Set the number of NICs and VNIs to 1 temporarily to avoid problems during the collective */
    MPIDI_OFI_global.num_vcis = MPIDI_OFI_global.num_nics = 1;

    /* Confirm that all processes have the same number of NICs */
    mpi_errno = MPIR_Allreduce_allcomm_auto(&tmp_num_nics, &num_nics, 1, MPIR_INT_INTERNAL,
                                            MPI_MIN, MPIR_Process.comm_world, MPIR_ERR_NONE);
    MPIDI_OFI_global.num_vcis = tmp_num_vcis;
    MPIDI_OFI_global.num_nics = tmp_num_nics;
    MPIR_ERR_CHECK(mpi_errno);

    /* If the user did not ask to fallback to fewer NICs, throw an error if someone is missing a
     * NIC. */
    if (tmp_num_nics != num_nics) {
        if (MPIR_CVAR_OFI_USE_MIN_NICS) {
            MPIDI_OFI_global.num_nics = num_nics;

            /* If we fall down to 1 nic, turn off multi-nic optimizations. */
            if (num_nics == 1) {
                MPIDI_OFI_COMM(MPIR_Process.comm_world).enable_striping = 0;
            }
        } else {
            MPIR_ERR_CHKANDJUMP(num_nics != MPIDI_OFI_global.num_nics, mpi_errno, MPI_ERR_OTHER,
                                "**ofi_num_nics");
        }
    }

    /* FIXME: It would also be helpful to check that all of the NICs can communicate so we can fall
     * back to other options if that is not the case (e.g., verbs are often configured with a
     * different subnet for each "set" of nics). It's unknown how to write a good check for that. */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int setup_additional_vcis(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int vci = 0; vci < MPIDI_OFI_global.num_vcis; vci++) {
        for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
            /* vci 0 nic 0 already created */
            if (vci > 0 || nic > 0) {
                mpi_errno = MPIDI_OFI_create_vci_context(vci, nic);
                if (mpi_errno != MPI_SUCCESS) {
                    /* running out of vcis, reduce MPIDI_OFI_global.num_vcis */
                    if (vci > 0) {
                        MPIDI_OFI_global.num_vcis = vci;
                        /* FIXME: destroy already created vci_context */
                        mpi_errno = MPI_SUCCESS;
                        goto fn_exit;
                    } else {
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                }
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* addr_exchange_all_ctx: exchange addresses for multiple vcis */

/* Macros to reduce clutter, so we can focus on the ordering logics.
 * Note: they are not perfectly wrapped, but tolerable since only used here. */

#if !defined(MPIDI_OFI_VNI_USE_DOMAIN) || MPIDI_CH4_MAX_VCIS == 1
/* NOTE: with scalable endpoint as context, all vcis share the same address. */
#define NUM_VCIS_FOR_RANK(r) 1
#else
#define NUM_VCIS_FOR_RANK(r) all_num_vcis[r]
#endif

#define GET_AV_AND_ADDRNAMES(rank) \
    MPIDI_OFI_addr_t *av ATTRIBUTE((unused)) = &MPIDI_OFI_AV(&MPIDIU_get_av(0, rank)); \
    char *r_names = all_names + rank * max_vcis * num_nics * name_len;

#define DO_AV_INSERT(ctx_idx, nic, vci) \
    fi_addr_t addr; \
    MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.ctx[ctx_idx].av, \
                                r_names + (vci * num_nics + nic) * name_len, 1, \
                                &addr, 0ULL, NULL), avmap);

#define SKIP_ROOT(nic, vci) \
    if (nic == 0 && vci == 0) { \
        continue; \
    }

/* with MPIDI_OFI_ENABLE_AV_TABLE, we potentially can omit storing av tables.
 * The following routines ensures we can do that. It is static now, but we can
 * easily export to global when we need to.
 */

ATTRIBUTE((unused))
static int get_root_av_table_index(int rank)
{
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        /* node roots with greater ranks are inserted before this rank if it a non-node-root */
        int num_extra = 0;

        /* check node roots */
        for (int i = 0; i < MPIR_Process.num_nodes; i++) {
            if (MPIR_Process.node_root_map[i] == rank) {
                return i;
            } else if (MPIR_Process.node_root_map[i] > rank) {
                num_extra++;
            }
        }

        /* must be non-node-root */
        return rank + num_extra;
    } else {
        return rank;
    }
}

ATTRIBUTE((unused))
static int get_av_table_index(int rank, int nic, int vci, int *all_num_vcis)
{
    if (nic == 0 && vci == 0) {
        return get_root_av_table_index(rank);
    } else {
        int num_nics = MPIDI_OFI_global.num_nics;
        int idx = 0;
        idx += MPIR_Process.size;       /* root entries */
        for (int i = 0; i < rank; i++) {
            idx += num_nics * NUM_VCIS_FOR_RANK(i) - 1;
        }
        idx += nic * NUM_VCIS_FOR_RANK(rank) + vci - 1;
        return idx;
    }
}

static int addr_exchange_all_ctx(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    MPIR_Comm *comm = MPIR_Process.comm_world;
    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;

    int max_vcis;
    int *all_num_vcis;

#if !defined(MPIDI_OFI_VNI_USE_DOMAIN) || MPIDI_CH4_MAX_VCIS == 1
    max_vcis = 1;
    all_num_vcis = NULL;
#else
    /* Allgather num_vcis */
    MPIR_CHKLMEM_MALLOC(all_num_vcis, sizeof(int) * size);
    mpi_errno = MPIR_Allgather_fallback(&MPIDI_OFI_global.num_vcis, 1, MPIR_INT_INTERNAL,
                                        all_num_vcis, 1, MPIR_INT_INTERNAL, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    max_vcis = 0;
    for (int i = 0; i < size; i++) {
        if (max_vcis < NUM_VCIS_FOR_RANK(i)) {
            max_vcis = NUM_VCIS_FOR_RANK(i);
        }
    }
#endif

    int num_vcis = NUM_VCIS_FOR_RANK(rank);
    int num_nics = MPIDI_OFI_global.num_nics;

    /* Assume num_nics are all equal */
    if (max_vcis * num_nics == 1) {
        goto fn_exit;
    }

    /* allocate additional av addrs */
    for (int i = 0; i < size; i++) {
        MPIDI_av_entry_t *av = &MPIDIU_get_av(0, i);
        MPIDI_OFI_AV(av).all_dest = MPL_malloc(max_vcis * num_nics * sizeof(fi_addr_t),
                                               MPL_MEM_ADDRESS);
        MPIR_ERR_CHKANDJUMP(!MPIDI_OFI_AV(av).all_dest, mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    /* libfabric uses uniform name_len within a single provider */
    int name_len = MPIDI_OFI_global.addrnamelen;
    int my_len = max_vcis * num_nics * name_len;
    char *all_names;
    MPIR_CHKLMEM_MALLOC(all_names, size * my_len);
    char *my_names = all_names + rank * my_len;

    /* put in my addrnames */
    for (int nic = 0; nic < num_nics; nic++) {
        for (int vci = 0; vci < num_vcis; vci++) {
            size_t actual_name_len = name_len;
            char *vci_addrname = my_names + (vci * num_nics + nic) * name_len;
            int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
            MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_OFI_global.ctx[ctx_idx].ep, vci_addrname,
                                      &actual_name_len), getname);
            MPIR_Assert(actual_name_len == name_len);
        }
    }
    /* Allgather */
    mpi_errno = MPIR_Allgather_fallback(MPI_IN_PLACE, 0, MPIR_BYTE_INTERNAL,
                                        all_names, my_len, MPIR_BYTE_INTERNAL, comm, MPIR_ERR_NONE);

    /* Step 2: insert and store non-root nic/vci on the root context */
    int root_ctx_idx = MPIDI_OFI_get_ctx_index(0, 0);
    for (int r = 0; r < size; r++) {
        GET_AV_AND_ADDRNAMES(r);
        for (int nic = 0; nic < num_nics; nic++) {
            for (int vci = 0; vci < NUM_VCIS_FOR_RANK(r); vci++) {
                SKIP_ROOT(nic, vci);
                DO_AV_INSERT(root_ctx_idx, nic, vci);
                MPIDI_OFI_AV_ADDR(av, 0, 0, vci, nic) = addr;
            }
        }
    }

    /* Step 3: insert all nic/vci on non-root context, following exact order as step 1 and 2 */

    int *is_node_roots = NULL;
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        MPIR_CHKLMEM_MALLOC(is_node_roots, size * sizeof(int));
        for (int r = 0; r < size; r++) {
            is_node_roots[r] = 0;
        }
        for (int i = 0; i < MPIR_Process.num_nodes; i++) {
            is_node_roots[MPIR_Process.node_root_map[i]] = 1;
        }
    }

    for (int nic_local = 0; nic_local < num_nics; nic_local++) {
        for (int vci_local = 0; vci_local < num_vcis; vci_local++) {
            SKIP_ROOT(nic_local, vci_local);
            int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, nic_local);

            /* -- same order as step 1 -- */
            if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
                /* node roots */
                for (int r = 0; r < size; r++) {
                    if (is_node_roots[r]) {
                        GET_AV_AND_ADDRNAMES(r);
                        DO_AV_INSERT(ctx_idx, 0, 0);
                        MPIR_Assert(MPIDI_OFI_AV_ROOT_ADDR(av) == addr);
                    }
                }
                /* non-node-root */
                for (int r = 0; r < size; r++) {
                    if (!is_node_roots[r]) {
                        GET_AV_AND_ADDRNAMES(r);
                        DO_AV_INSERT(ctx_idx, 0, 0);
                        MPIR_Assert(MPIDI_OFI_AV_ROOT_ADDR(av) == addr);
                    }
                }
            } else {
                /* !MPIR_CVAR_CH4_ROOTS_ONLY_PMI */
                for (int r = 0; r < size; r++) {
                    GET_AV_AND_ADDRNAMES(r);
                    DO_AV_INSERT(ctx_idx, 0, 0);
                    MPIR_Assert(MPIDI_OFI_AV_ROOT_ADDR(av) == addr);
                }
            }

            /* -- same order as step 2 -- */
            for (int r = 0; r < size; r++) {
                GET_AV_AND_ADDRNAMES(r);
                for (int nic = 0; nic < num_nics; nic++) {
                    for (int vci = 0; vci < NUM_VCIS_FOR_RANK(r); vci++) {
                        SKIP_ROOT(nic, vci);
                        DO_AV_INSERT(ctx_idx, nic, vci);
                        MPIR_Assert(MPIDI_OFI_AV_ADDR(av, 0, 0, vci, nic) == addr);
                    }
                }
            }
        }
    }
    mpi_errno = MPIR_Barrier_fallback(comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* check */
#if MPIDI_CH4_MAX_VCIS > 1
    if (MPIDI_OFI_ENABLE_AV_TABLE) {
        for (int r = 0; r < size; r++) {
            MPIDI_OFI_addr_t *av ATTRIBUTE((unused)) = &MPIDI_OFI_AV(&MPIDIU_get_av(0, r));
            for (int nic = 0; nic < num_nics; nic++) {
                for (int vci = 0; vci < NUM_VCIS_FOR_RANK(r); vci++) {
                    MPIR_Assert(MPIDI_OFI_AV_ADDR(av, 0, 0, vci, nic) ==
                                get_av_table_index(r, nic, vci, all_num_vcis));
                }
            }
        }
    }
#endif
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
