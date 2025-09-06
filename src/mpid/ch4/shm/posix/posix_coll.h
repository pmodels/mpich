/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_COLL_H_INCLUDED
#define POSIX_COLL_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "posix_coll_release_gather.h"
#include "posix_coll_gpu_ipc.h"
#include "posix_csel_container.h"


/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node bcast
        mpir           - Fallback to MPIR collectives
        release_gather - Force shm optimized algo using release, gather primitives
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE)
        ipc_read - Uses read-based collective with ipc

    - name        : MPIR_CVAR_IBCAST_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node bcast
        mpir           - Fallback to MPIR collectives
        release_gather - Force shm optimized algo using release, gather primitives
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE)

    - name        : MPIR_CVAR_REDUCE_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node reduce
        mpir           - Fallback to MPIR collectives
        release_gather - Force shm optimized algo using release, gather primitives
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE)

    - name        : MPIR_CVAR_IREDUCE_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node reduce
        mpir           - Fallback to MPIR collectives
        release_gather - Force shm optimized algo using release, gather primitives
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE)

    - name        : MPIR_CVAR_ALLREDUCE_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node allreduce
        mpir           - Fallback to MPIR collectives
        release_gather - Force shm optimized algo using release, gather primitives
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE)

    - name        : MPIR_CVAR_BARRIER_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      group       : MPIR_CVAR_GROUP_COLL_ALGO
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node barrier
        mpir           - Fallback to MPIR collectives
        release_gather - Force shm optimized algo using release, gather primitives
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE)

    - name        : MPIR_CVAR_ALLTOALL_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : mpir
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node alltoall
        mpir           - Fallback to MPIR collectives (default)
        ipc_read    - Uses read-based collective with ipc

    - name        : MPIR_CVAR_ALLGATHER_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : mpir
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node allgather
        mpir        - Fallback to MPIR collectives (default)
        ipc_read    - Uses read-based collective with ipc

    - name        : MPIR_CVAR_ALLGATHERV_POSIX_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : mpir
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node allgatherv
        mpir        - Fallback to MPIR collectives (default)
        ipc_read    - Uses read-based collective with ipc

    - name        : MPIR_CVAR_POSIX_POLL_FREQUENCY
      category    : COLLECTIVE
      type        : int
      default     : 1000
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This cvar sets the number of loops before the yield function is called.  A
        value of 0 disables yielding.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#endif /* POSIX_COLL_H_INCLUDED */
