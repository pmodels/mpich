/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4_proc.h"
#include "ch4_coll_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BARRIER_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Barrier
        0 Auto selection
        1 NM + SHM
        2 NM only

    - name        : MPIR_CVAR_BCAST_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Bcast
        0 Auto selection
        1 NM + SHM with explicit send-recv between rank 0 and root
        2 NM + SHM without the explicit send-recv
        3 NM only

    - name        : MPIR_CVAR_ALLREDUCE_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Allreduce
        0 Auto selection
        1 NM + SHM with reduce + bcast
        2 NM only composition
        3 SHM only composition
        4 Multi leaders based inter node + intra node composition

    - name        : MPIR_CVAR_ALLGATHER_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Allgather
        0 Auto selection
        1 Multi leaders based inter node + intra node composition
        2 NM only composition

    - name        : MPIR_CVAR_ALLTOALL_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Alltoall
        0 Auto selection
        1 Multi leaders based inter node + intra node composition
        2 NM only composition

    - name        : MPIR_CVAR_REDUCE_COMPOSITION
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Select composition (inter_node + intra_node) for Reduce
        0 Auto selection
        1 NM + SHM with explicit send-recv between rank 0 and root
        2 NM + SHM without the explicit send-recv
        3 NM only

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX void MPIDI_Allreduce_fill_multi_leads_info(MPIR_Comm * comm)
{
    if (MPIDI_COMM(comm, allreduce_comp_info) == NULL) {
        /* If multi-leads allreduce is enabled and the info object is not yet set */
        MPIDI_COMM(comm, allreduce_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, allreduce_comp_info));
        MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLREDUCE(comm, shm_addr) = NULL;
        MPIDI_COMM(comm, shm_size_per_lead) = 1;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) == -1) {
        if (MPII_Comm_is_node_balanced(comm)) {
            MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) = 1;
        } else {
            MPIDI_COMM_ALLREDUCE(comm, use_multi_leads) = 0;
        }
    }

}

MPL_STATIC_INLINE_PREFIX void MPIDI_Allgather_fill_multi_leads_info(MPIR_Comm * comm)
{
    if (MPIDI_COMM(comm, allgather_comp_info) == NULL) {
        MPIDI_COMM(comm, allgather_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, allgather_comp_info));
        MPIDI_COMM_ALLGATHER(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLGATHER(comm, shm_addr) = NULL;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLGATHER(comm, use_multi_leads) == -1) {
        if (MPII_Comm_is_node_canonical(comm))
            MPIDI_COMM_ALLGATHER(comm, use_multi_leads) = 1;
        else
            MPIDI_COMM_ALLGATHER(comm, use_multi_leads) = 0;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_Alltoall_fill_multi_leads_info(MPIR_Comm * comm)
{
    if (MPIDI_COMM(comm, alltoall_comp_info) == NULL) {
        MPIDI_COMM(comm, alltoall_comp_info) =
            MPL_malloc(sizeof(MPIDI_Multileads_comp_info_t), MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_COMM(comm, alltoall_comp_info));
        MPIDI_COMM_ALLTOALL(comm, use_multi_leads) = -1;
        MPIDI_COMM_ALLTOALL(comm, shm_addr) = NULL;
    }
    /* Find if the comm meets the constraints and store that info in the data structure */
    if (MPIDI_COMM_ALLTOALL(comm, use_multi_leads) == -1) {
        if (MPII_Comm_is_node_canonical(comm))
            MPIDI_COMM_ALLTOALL(comm, use_multi_leads) = 1;
        else
            MPIDI_COMM_ALLTOALL(comm, use_multi_leads) = 0;
    }
}

#endif /* CH4_COLL_H_INCLUDED */
