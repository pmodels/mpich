/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"

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
    char *addrname[FI_NAME_MAX];
    size_t addrnamelen = FI_NAME_MAX;
    MPIDI_OFI_CALL(fi_getname((fid_t) MPIDI_OFI_global.ctx[0].ep, addrname, &addrnamelen), getname);

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
                MPIR_Assert(MPIDI_OFI_AV_ADDR_ROOT(av) != FI_ADDR_NOTAVAIL);
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
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
