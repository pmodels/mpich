/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_RNDV_H_INCLUDED
#define OFI_RNDV_H_INCLUDED

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL
      category    : CH4_OFI
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : |-
        When message size is greater than MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD,
        specify large message protocol.
        auto - decide protocols based on buffer attributes and datatypes.
        pipeline - use pipeline protocol (forcing pack and unpack).
        read - RDMA read.
        write - RDMA write.
        direct - direct send data using libfabric after the RNDV handshake.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* declare send/recv functions for rndv protocols */

#endif /* OFI_RNDV_H_INCLUDED */
