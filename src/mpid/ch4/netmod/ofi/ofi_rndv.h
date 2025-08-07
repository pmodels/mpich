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

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    MPIR_Request *req;          /* sreq or rreq */
    char hdr[];
} MPIDI_OFI_RNDV_control_req_t;

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_RNDV_send_hdr(void *hdr, int hdr_sz, MPIDI_av_entry_t * av,
                                                     int vci_local, int vci_remote,
                                                     uint64_t match_bits)
{
    int mpi_errno = MPI_SUCCESS;

    /* control message always use nic 0 */
    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, 0);
    fi_addr_t addr = MPIDI_OFI_av_to_phys(av, vci_local, 0, vci_remote, 0);
    MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                    hdr, hdr_sz, addr, match_bits), vci_local, tinject);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_RNDV_recv_hdr(MPIR_Request * parent_request, int event_id,
                                                     int hdr_sz, MPIDI_av_entry_t * av,
                                                     int vci_local, int vci_remote,
                                                     uint64_t match_bits)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_RNDV_control_req_t *control;
    control = MPL_malloc(sizeof(MPIDI_OFI_RNDV_control_req_t) + hdr_sz, MPL_MEM_OTHER);
    MPIR_Assertp(control);

    control->event_id = event_id;
    control->req = parent_request;

    /* control message always use nic 0 */
    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, 0);
    fi_addr_t addr = MPIDI_OFI_av_to_phys(av, vci_local, 0, vci_remote, 0);

    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                  control->hdr, hdr_sz, NULL,
                                  addr, match_bits, 0ULL, (void *) &control->context),
                         vci_local, trecv);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define MPIDI_OFI_RNDV_GET_CONTROL_HDR(r) ((void *)((MPIDI_OFI_RNDV_control_req_t *)(r))->hdr)
#define MPIDI_OFI_RNDV_GET_CONTROL_REQ(r) ((MPIDI_OFI_RNDV_control_req_t *)(r))->req

#endif /* OFI_RNDV_H_INCLUDED */
