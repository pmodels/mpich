/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_init.h"
#include "mpidu_bc.h"

/* Perform address exchange and enable netmod communication over the comm. */

static int create_node_comm(MPIR_Comm * comm, MPIR_Comm ** node_comm_out);
static int create_node_roots_comm(MPIR_Comm * comm, MPIR_Comm ** node_roots_comm_out);

/* NOTE: we assume the intranode shm communication is already enabled.
 * NOTE: the root av no longer require fixed insertion order. See comments in ofi_vci.c.
 * FIXME: need survey whether we need address exchange and only insert missing ones
 */
int MPIDI_OFI_comm_addr_exchange(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    /* only comm_world for now */
    MPIR_Assert(comm == MPIR_Process.comm_world);

    /* First, each get its own name */
    MPIDI_OFI_global.addrnamelen = FI_NAME_MAX;
    MPIDI_OFI_CALL(fi_getname
                   ((fid_t) MPIDI_OFI_global.ctx[0].ep, MPIDI_OFI_global.addrname[0],
                    &MPIDI_OFI_global.addrnamelen), getname);

    char *addrname = MPIDI_OFI_global.addrname[0];
    int addrnamelen = MPIDI_OFI_global.addrnamelen;

    /* - create node_comm and node_roots_comm.
     * * node_comm is already active via shm.
     * * node_roots_comm will be activated after PMI allgather and av_inserts
     */
    MPIR_Comm *node_comm;
    mpi_errno = create_node_comm(comm, &node_comm);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm *node_roots_comm;
    mpi_errno = create_node_roots_comm(comm, &node_roots_comm);
    MPIR_ERR_CHECK(mpi_errno);

    int local_rank = node_comm->rank;
    int local_size = node_comm->local_size;
    int external_size = node_roots_comm->local_size;

    /* PMI allgather over node roots and av_insert to activate node_roots_comm */
    if (local_rank == 0) {
        char *roots_names;
        MPIR_CHKLMEM_MALLOC(roots_names, external_size * addrnamelen);

        MPIR_PMI_DOMAIN domain = MPIR_PMI_DOMAIN_NODE_ROOTS;
        mpi_errno = MPIR_pmi_allgather(addrname, addrnamelen, roots_names, addrnamelen, domain);
        MPIR_ERR_CHECK(mpi_errno);

        /* insert av and activate node_roots_comm */
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

    comm->node_comm = node_comm;
    comm->node_roots_comm = node_roots_comm;
    mpi_errno = MPIR_Allgather_intra_smp_no_order(my_rankname, rankname_len, MPIR_BYTE_INTERNAL,
                                                  all_ranknames, rankname_len, MPIR_BYTE_INTERNAL,
                                                  comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    comm->node_comm = NULL;
    comm->node_roots_comm = NULL;

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

/* -- static internal routines -- */

/* TODO: make this MPIR-layer utility, make this lightweight, so we can call this lazily and on-demand */
static int create_node_comm(MPIR_Comm * comm, MPIR_Comm ** node_comm_out)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *subcomm;

    int num_local, local_rank, *local_procs;
    mpi_errno = MPIR_Find_local(comm, &num_local, &local_rank, &local_procs, NULL);

    mpi_errno = MPIR_Comm_create(&subcomm);
    MPIR_ERR_CHECK(mpi_errno);

    subcomm->context_id = comm->context_id + MPIR_CONTEXT_INTRANODE_OFFSET;
    subcomm->recvcontext_id = subcomm->context_id;
    subcomm->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    subcomm->hierarchy_kind = MPIR_COMM_HIERARCHY_KIND__NODE;

    subcomm->rank = local_rank;
    subcomm->local_size = num_local;
    subcomm->remote_size = num_local;

    /* construct local_group */
    MPIR_Group *parent_group = comm->local_group;
    MPIR_Assert(parent_group);
    mpi_errno = MPIR_Group_incl_impl(parent_group, num_local, local_procs, &subcomm->local_group);
    MPIR_ERR_CHECK(mpi_errno);

    *node_comm_out = subcomm;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_node_roots_comm(MPIR_Comm * comm, MPIR_Comm ** node_roots_comm_out)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *subcomm;

    int num_external, external_rank, *external_procs;
    mpi_errno = MPIR_Find_external(comm, &num_external, &external_rank, &external_procs, NULL);

    mpi_errno = MPIR_Comm_create(&subcomm);
    MPIR_ERR_CHECK(mpi_errno);

    subcomm->context_id = comm->context_id + MPIR_CONTEXT_INTRANODE_OFFSET;
    subcomm->recvcontext_id = subcomm->context_id;
    subcomm->comm_kind = MPIR_COMM_KIND__INTRACOMM;
    subcomm->hierarchy_kind = MPIR_COMM_HIERARCHY_KIND__NODE;

    subcomm->rank = external_rank;
    subcomm->local_size = num_external;
    subcomm->remote_size = num_external;

    /* construct local_group */
    MPIR_Group *parent_group = comm->local_group;
    MPIR_Assert(parent_group);
    mpi_errno = MPIR_Group_incl_impl(parent_group, num_external, external_procs,
                                     &subcomm->local_group);
    MPIR_ERR_CHECK(mpi_errno);

    *node_roots_comm_out = subcomm;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
