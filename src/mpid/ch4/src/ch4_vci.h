/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_VCI_H_INCLUDED
#define CH4_VCI_H_INCLUDED

#include "ch4_impl.h"

/* vci is embedded in the request's pool index */

#define MPIDI_Request_get_vci(req) MPIR_REQUEST_POOL(req)
#define MPIDI_VCI_INVALID (-1)

/* VCI hashing function (fast path)
 * NOTE: The returned vci should always MOD NUMVCIS, where NUMVCIS is
 *       the number of VCIs determined at init time
 *       Potentially, we'd like to make it config constants of power of 2
 * TODO: move the MOD here.
 */

/* For consistent hashing, we may need differentiate between src and dst vci and whether
 * it is being called from sender side or receiver side (consdier intercomm). We use an
 * integer flag to encode the information.
 *
 * The flag constants are designed as bit fields, so different hashing algorithm can easily
 * pick the needed information.
 */
#define SRC_VCI_FROM_SENDER 0   /* Bit 1 marks SRC/DST */
#define DST_VCI_FROM_SENDER 1
#define SRC_VCI_FROM_RECVER 2   /* Bit 2 marks SENDER/RECVER */
#define DST_VCI_FROM_RECVER 3

#if MPIDI_CH4_VCI_METHOD == MPICH_VCI__ZERO

/* Always and only use vci 0, equivalent to NUM_VCIS==1 */
MPL_STATIC_INLINE_PREFIX int MPIDI_get_vci(int flag, MPIR_Comm * comm_ptr,
                                           int src_rank, int dst_rank, int tag)
{
    return 0;
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__COMM

/* One communicator per vci. VCI assigned at communitor creation */
MPL_STATIC_INLINE_PREFIX int MPIDI_get_vci(int flag, MPIR_Comm * comm_ptr,
                                           int src_rank, int dst_rank, int tag)
{
    return comm_ptr->seq;
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__TAG
#error "MPICH_VCI__TAG not implemented."
/* A way to allow explicit vci by user, based on (undocumented) convention.
   Embed src_vci and dst_vci inside tag, 5 bits each
   NOTE: this serve as temporary replacement for explicit scheme (until mpi-layer api adds support).
   TODO: ways to allow customization of bit schemes
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_get_vci(int flag, MPIR_Comm * comm_ptr,
                                           int src_rank, int dst_rank, int tag)
{
    if (!(flag & 0x1)) {
        /* src */
        return (tag == MPI_ANY_TAG) ? 0 : ((tag >> 10) & 0x1f);
    } else {
        /* dst */
        return (tag == MPI_ANY_TAG) ? 0 : ((tag >> 5) & 0x1f);
    }
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__IMPLICIT

static bool is_vci_restricted_to_zero(MPIR_Comm * comm)
{
    bool vci_restricted = false;
    if (!(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM && !comm->tainted)) {
        vci_restricted |= true;
    }

    return vci_restricted;
}


/* Return VCI index of a send transmit context.
 * Used for two purposes:
 *   1. For the sender side to determine which VCI index of a transmit context
 *      to send a message from
 *   2. For the receiver side to determine which VCI of the remote peer to address
 *      to when sending ack for sync sends
 * Note: the unused parameters can be used in future
 *
 * ctxid_in_effect: communicator's context ID used to calculate the VCI index.
 *   For an intercommunicator, the right one may be either comm->context_id
 *   or comm->recvcontext_id, depending on situation.
 *   This parameter allows caller to explicitly specify context ID.
 *
 * When this function is called from the sender side, ctxid_in_effect should be comm->context_id.
 * Otherwise (receiver side), it should be comm->recvcontext_id.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_get_sender_vci(MPIR_Comm * comm,
                                                  MPIR_Context_id_t ctxid_in_effect,
                                                  int sender_rank, int receiver_rank, int tag)
{
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    MPIR_Assert(comm);
    int vci_idx = MPIDI_VCI_INVALID;
    bool use_user_defined_vci = (comm->hints[MPIR_COMM_HINT_SENDER_VCI] != MPIDI_VCI_INVALID);
    if (is_vci_restricted_to_zero(comm)) {
        vci_idx = 0;
    } else if (use_user_defined_vci) {
        vci_idx = comm->hints[MPIR_COMM_HINT_SENDER_VCI];
    } else {
        /* TODO: implement implicit hashing using other parameters */
        vci_idx = comm->seq;
    }
    return vci_idx;
#else
    return 0;
#endif
}

/* Return VCI index of a receive transmit context.
 * Used for two purposes:
 *   1. For the receive side to determine where to post a receive call
 *   2. For the sender side to determine which VCI in the remote peer to address to
 * Note: the unused parameters can be used in future
 *
 * ctxid_in_effect: communicator's context ID used to calculate the VCI index.
 *   For an intercommunicator, the right one may be either comm->context_id
 *   or comm->recvcontext_id, depending on situation.
 *   This parameter allows caller to explicitly specify context ID.
 *
 * When this function is called from the sender side, ctxid_in_effect should be comm->context_id.
 * Otherwise (receiver side), it should be comm->recvcontext_id.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_get_receiver_vci(MPIR_Comm * comm,
                                                    MPIR_Context_id_t ctxid_in_effect,
                                                    int sender_rank, int receiver_rank, int tag)
{
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    MPIR_Assert(comm);
    int vci_idx = MPIDI_VCI_INVALID;
    bool use_user_defined_vci = (comm->hints[MPIR_COMM_HINT_RECEIVER_VCI] != MPIDI_VCI_INVALID);
    if (is_vci_restricted_to_zero(comm)) {
        vci_idx = 0;
    } else if (use_user_defined_vci) {
        vci_idx = comm->hints[MPIR_COMM_HINT_RECEIVER_VCI];
    } else {
        /* TODO: implement implicit hashing using other parameters */
        vci_idx = comm->seq;
    }
    return vci_idx;
#else
    return 0;
#endif
}


/* Figure out vci based on (comm, rank, tag) plus hints
 * This is essentially an "auto" method, we use "implicit" as a contrast * to "explicit", which could be available with, e.g. MPI endpoints.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_get_vci(int flag, MPIR_Comm * comm_ptr,
                                           int src_rank, int dst_rank, int tag)
{
    int ctxid_in_effect;
    if (!(flag & 0x2)) {
        /* called from sender */
        ctxid_in_effect = comm_ptr->context_id;
    } else {
        /* called from receiver */
        ctxid_in_effect = comm_ptr->recvcontext_id;
    }

    if (!(flag & 0x1)) {
        /* src */
        return MPIDI_get_sender_vci(comm_ptr, ctxid_in_effect, src_rank, dst_rank, tag);
    } else {
        /* dst */
        return MPIDI_get_receiver_vci(comm_ptr, ctxid_in_effect, src_rank, dst_rank, tag);
    }
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__EXPLICIT
/* User set explicit src_vci/dst_vci per operation, possibly with
 * MPIX_Send/Recv(..., int src_vci, int dst_vci) or MPI endpoints.
 */
#error "MPICH_VCI__EXPLICIT not implemented."
MPL_STATIC_INLINE_PREFIX int MPIDI_get_vci(int flag, MPIR_Comm * comm_ptr,
                                           int src_rank, int dst_rank, int tag)
{
    return 0;
}

#else
#error Invalid MPIDI_CH4_VCI_METHOD!
#endif

#endif /* CH4_VCI_H_INCLUDED */
