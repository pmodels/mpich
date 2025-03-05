/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

/* NOTE on av insertion order:
 *
 * Each nic-vci is an endpoint with a unique address, and inside libfabric maintains
 * one av table. Thus to fully store the address mapping, we'll need a multi-dim table as
 *     av_table[src_vci][src_nic][dest_rank][dest_vci][dest_nic]
 * Note, this table is for illustration, and different from MPIDI_OFI_addr_t.
 *
 * However, if we insert the addresses in the same order between local endpoints, then the
 * av table indexes will be *identical*. Then, we can omit the dimension of [src_vci][src_nic].
 * I.e. we only need av_table[rank][vci][nic], saving two dimensions of local vcis and local nics.
 *
 * To achieve that, we need always insert each remote address on *all* local endpoints together.
 * Because we separate root addr (av_table[0][0][rank][0][0]) separately, we allow the root
 * address to be inserted separately from the rest. The rest of the addresses are only
 * needed when multiple vcis/nics are enabled. But we require for each remote rank, all remote
 * endpoints to be inserted all at once.
 */

int MPIDI_OFI_vci_init(void)
{
    MPIDI_OFI_global.num_nics = 1;
    MPIDI_OFI_global.num_vcis = 1;
    return MPI_SUCCESS;
}

/* Enable multiple VCIs and perform address exchange within comm */

static int check_num_nics(MPIR_Comm * comm, int *num_nics_out);
static int setup_additional_vcis(int num_vcis, int num_nics, int *actual_num_vcis);
static int addr_exchange_all_ctx(MPIR_Comm * comm, int *all_num_vcis);

int MPIDI_OFI_comm_set_vcis(MPIR_Comm * comm, int num_vcis, int *all_num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    /* NOTE: we only can create contexts for additional vcis and nics once (see notes at top) */
    /* TODO: relax it as long as num_{vcis,nics} <= MPIDI_OFI_global.num_{vcis,nics}, which we skip the creation part */
    MPIR_Assert(MPIDI_OFI_global.num_vcis == 1 && MPIDI_OFI_global.num_nics == 1);

    /* Multiple vci without using domain require MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS */
#ifndef MPIDI_OFI_VNI_USE_DOMAIN
    MPIR_Assert(num_vcis == 1 || MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS);
#endif

    /* All processes must have the same number of NICs */
    int num_nics;
    mpi_errno = check_num_nics(comm, &num_nics);
    MPIR_ERR_CHECK(mpi_errno);

    int actual_num_vcis;
    /* NOOP unless num_vcis > 1 or num_nics > 1 */
    mpi_errno = setup_additional_vcis(num_vcis, num_nics, &actual_num_vcis);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_global.num_nics = num_nics;
    MPIDI_OFI_global.num_vcis = actual_num_vcis;

    if (MPIR_CVAR_DEBUG_SUMMARY && comm->rank == 0) {
        printf("==== MPIDI_OFI_comm_set_vcis ====\n");
        printf("num_vcis: %d\n", MPIDI_OFI_global.num_vcis);
        printf("num_nics: %d\n", MPIDI_OFI_global.num_nics);
        printf("======================================\n");
    }

    mpi_errno = MPIR_Allgather_impl(&MPIDI_OFI_global.num_vcis, 1, MPI_INT,
                                    all_num_vcis, 1, MPI_INT, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* NOOP unless one of the all_num_vcis[i] > 1 or num_nics > 1 */
    mpi_errno = addr_exchange_all_ctx(comm, all_num_vcis);
    MPIR_ERR_CHECK(mpi_errno);

    for (int vci = 1; vci < MPIDI_OFI_global.num_vcis; vci++) {
        MPIDI_OFI_am_init(vci);
        MPIDI_OFI_am_post_recv(vci, 0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int check_num_nics(MPIR_Comm * comm, int *num_nics_out)
{
    int mpi_errno = MPI_SUCCESS;

    int tmp_num_nics = MPIDI_OFI_global.num_nics_available;
    mpi_errno = MPIR_Allreduce_allcomm_auto(&tmp_num_nics, num_nics_out, 1, MPIR_INT_INTERNAL,
                                            MPI_MIN, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* If the user did not ask to fallback to fewer NICs, throw an error if someone is missing a
     * NIC. */
    if (tmp_num_nics != *num_nics_out) {
        if (!MPIR_CVAR_OFI_USE_MIN_NICS) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ofi_num_nics");
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int setup_additional_vcis(int num_vcis, int num_nics, int *actual_num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    *actual_num_vcis = num_vcis;
    for (int vci = 0; vci < num_vcis; vci++) {
        for (int nic = 0; nic < num_nics; nic++) {
            /* vci 0 nic 0 already created */
            if (vci > 0 || nic > 0) {
                mpi_errno = MPIDI_OFI_create_vci_context(vci, nic);
                if (mpi_errno != MPI_SUCCESS) {
                    /* running out of vcis, reduce MPIDI_OFI_global.num_vcis */
                    if (vci > 0) {
                        *actual_num_vcis = vci;
                        /* FIXME: destroy already created vci_context */
                        mpi_errno = MPI_SUCCESS;
                        goto fn_exit;
                    } else {
                        /* fatal error if we can not enable all nics on vci 0 */
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
    MPIDI_av_entry_t *av ATTRIBUTE((unused)) = &MPIDIU_get_av(0, rank); \
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

static int addr_exchange_all_ctx(MPIR_Comm * comm, int *all_num_vcis)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int nprocs = comm->local_size;
    int myrank = comm->rank;

    /* get the max_vcis (so we can do Allgather rather than Allgatherv) */
    int max_vcis = 0;
    for (int i = 0; i < nprocs; i++) {
        if (max_vcis < NUM_VCIS_FOR_RANK(i)) {
            max_vcis = NUM_VCIS_FOR_RANK(i);
        }
    }

    int my_num_vcis = NUM_VCIS_FOR_RANK(comm->rank);
    int num_nics = MPIDI_OFI_global.num_nics;

    /* Assume num_nics are all equal */
    if (max_vcis * num_nics == 1) {
        goto fn_exit;
    }

    /* allocate all_dest[] in av entry */
    for (int i = 0; i < nprocs; i++) {
        MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, i);
        MPIR_Assert(MPIDI_OFI_AV(av).all_dest == NULL);
        MPIDI_OFI_AV(av).all_dest = MPL_malloc(max_vcis * num_nics * sizeof(fi_addr_t),
                                               MPL_MEM_ADDRESS);
        MPIR_ERR_CHKANDJUMP(!MPIDI_OFI_AV(av).all_dest, mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    /* libfabric uses uniform name_len within a single provider */
    int name_len = MPIDI_OFI_global.addrnamelen;
    int my_len = max_vcis * num_nics * name_len;
    char *all_names;
    MPIR_CHKLMEM_MALLOC(all_names, nprocs * my_len);
    char *my_names = all_names + myrank * my_len;

    /* put in my addrnames */
    for (int nic = 0; nic < num_nics; nic++) {
        for (int vci = 0; vci < my_num_vcis; vci++) {
            size_t actual_name_len = name_len;
            char *vci_addrname = my_names + (vci * num_nics + nic) * name_len;
            int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
            MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_OFI_global.ctx[ctx_idx].ep, vci_addrname,
                                      &actual_name_len), getname);
            MPIR_Assert(actual_name_len == name_len);
        }
    }
    /* Allgather */
    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_BYTE,
                                    all_names, my_len, MPI_BYTE, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* insert and store non-root nic/vci on the root context */
    for (int r = 0; r < nprocs; r++) {
        fi_addr_t expect_addr = FI_ADDR_NOTAVAIL;
        fi_addr_t root_offset = 0;
        GET_AV_AND_ADDRNAMES(r);
        /* for each remote endpoints */
        for (int nic = 0; nic < num_nics; nic++) {
            for (int vci = 0; vci < NUM_VCIS_FOR_RANK(r); vci++) {
                /* for each local endpoints */
                for (int nic_local = 0; nic_local < num_nics; nic_local++) {
                    for (int vci_local = 0; vci_local < my_num_vcis; vci_local++) {
                        /* skip root */
                        if (nic == 0 && vci == 0 && nic_local == 0 && vci_local == 0) {
                            continue;
                        }
                        int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, nic_local);
                        DO_AV_INSERT(ctx_idx, nic, vci);
                        /* we expect all resulting addr to be the same except for local root endpoint, which
                         * will have an offset */
                        if (expect_addr == FI_ADDR_NOTAVAIL) {
                            expect_addr = addr;
                        } else if (nic_local == 0 && vci_local == 0) {
                            if (root_offset == 0) {
                                root_offset = addr - expect_addr;
                            } else {
                                MPIR_Assert(addr == expect_addr + root_offset);
                            }
                        } else {
                            MPIR_Assert(addr == expect_addr);
                        }
                    }
                }
                MPIR_Assert(expect_addr != FI_ADDR_NOTAVAIL);
                MPIDI_OFI_AV_ADDR_NO_OFFSET(av, vci, nic) = expect_addr;
                /* next */
                expect_addr++;
            }
        }
        MPIDI_OFI_AV(av).root_offset = root_offset;
    }

    mpi_errno = MPIR_Barrier_fallback(comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
