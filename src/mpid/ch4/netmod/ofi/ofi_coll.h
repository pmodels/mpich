/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_COLL_H_INCLUDED
#define OFI_COLL_H_INCLUDED

#include "ofi_impl.h"
#include "coll/ofi_bcast_tree_tagged.h"
#include "coll/ofi_bcast_tree_rma.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_OFI_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select algorithm for intra-node bcast
        mpir                        - Fallback to MPIR collectives
        trigger_tree_tagged         - Force triggered ops based Tagged Tree
        trigger_tree_rma            - Force triggered ops based RMA Tree
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_CH4_OFI_COLL_SELECTION_TUNING_JSON_FILE)
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#endif /* OFI_COLL_H_INCLUDED */
