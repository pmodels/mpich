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

/* Check for explicit vcis (stream comm and direct attr) */
#define MPIDI_EXPLICIT_VCIS(comm, attr, src_rank, dst_rank, vci_src, vci_dst) \
    do { \
        if ((comm)->stream_comm_type == MPIR_STREAM_COMM_NONE) { \
            vci_src = 0; \
            vci_dst = 0; \
        } else if ((comm)->stream_comm_type == MPIR_STREAM_COMM_SINGLE) { \
            vci_src = (comm)->stream_comm.single.vci_table[src_rank]; \
            vci_dst = (comm)->stream_comm.single.vci_table[dst_rank]; \
        } else if ((comm)->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) { \
            if (MPIR_PT2PT_ATTR_HAS_VCI(attr)) { \
                vci_src = MPIR_PT2PT_ATTR_SRC_VCI(attr); \
                vci_dst = MPIR_PT2PT_ATTR_DST_VCI(attr); \
            } else { \
                int src_displ = (comm)->stream_comm.multiplex.vci_displs[src_rank]; \
                int dst_displ = (comm)->stream_comm.multiplex.vci_displs[dst_rank]; \
                vci_src = (comm)->stream_comm.multiplex.vci_table[src_displ]; \
                vci_dst = (comm)->stream_comm.multiplex.vci_table[dst_displ]; \
            } \
        } else { \
            MPIR_Assert(0); \
            vci_src = 0; \
            vci_dst = 0; \
        } \
    } while (0)

/* vci above implicit vci pool are always explciitly allocated by user. It is
 * always under serial execution context and we can skip thread synchronizations.
 */
#define MPIDI_VCI_IS_EXPLICIT(vci) (vci >= MPIDI_global.n_vcis)

/* VCI hashing function (fast path) */

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
    return comm_ptr->seq % MPIDI_global.n_vcis;
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__TAG
/* A way to allow explicit vci by user, based on (undocumented) convention.
   Embed src_vci and dst_vci inside tag, 5 bits each
   NOTE: this serve as temporary replacement for explicit scheme (until mpi-layer api adds support).
   TODO: ways to allow customization of bit schemes
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_get_vci(int flag, MPIR_Comm * comm_ptr,
                                           int src_rank, int dst_rank, int tag)
{
    int vci;
    if (!(flag & 0x1)) {
        /* src */
        vci = (tag == MPI_ANY_TAG) ? 0 : ((tag >> 10) & 0x1f);
    } else {
        /* dst */
        vci = (tag == MPI_ANY_TAG) ? 0 : ((tag >> 5) & 0x1f);
    }
    return vci % MPIDI_global.n_vcis;
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__IMPLICIT

/* Map comm to vci_idx */
MPL_STATIC_INLINE_PREFIX int MPIDI_map_contextid_to_vci(MPIR_Context_id_t context_id)
{
    return (MPIR_CONTEXT_READ_FIELD(PREFIX, context_id)) % MPIDI_global.n_vcis;
}

/* Map comm and rank to vci_idx */
MPL_STATIC_INLINE_PREFIX int MPIDI_map_contextid_rank_to_vci(MPIR_Context_id_t context_id, int rank)
{
    return (MPIR_CONTEXT_READ_FIELD(PREFIX, context_id) + rank) % MPIDI_global.n_vcis;
}

/* Map comm and tag to vci_idx */
MPL_STATIC_INLINE_PREFIX int MPIDI_map_contextid_tag_to_vci(MPIR_Context_id_t context_id, int tag)
{
    return (MPIR_CONTEXT_READ_FIELD(PREFIX, context_id) + tag) % MPIDI_global.n_vcis;;
}

/* Map comm, rank, and tag to vci_idx */
MPL_STATIC_INLINE_PREFIX int MPIDI_map_contextid_rank_tag_to_vci(MPIR_Context_id_t context_id,
                                                                 int rank, int tag)
{
    return (MPIR_CONTEXT_READ_FIELD(PREFIX, context_id) + rank + tag) % MPIDI_global.n_vcis;;
}

static bool is_vci_restricted_to_zero(MPIR_Comm * comm)
{
    bool vci_restricted = false;
    if (!(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM && !comm->tainted)) {
        vci_restricted |= true;
    }
    if (!MPIDI_global.is_initialized) {
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
    bool use_tag = comm->hints[MPIR_COMM_HINT_NO_ANY_TAG];

    if (is_vci_restricted_to_zero(comm)) {
        vci_idx = 0;
    } else if (use_user_defined_vci) {
        vci_idx = comm->hints[MPIR_COMM_HINT_SENDER_VCI];
    } else {
        if (use_tag) {
            vci_idx = MPIDI_map_contextid_rank_tag_to_vci(ctxid_in_effect, receiver_rank, tag);
        } else {
            /* General unoptimized case */
            vci_idx = MPIDI_map_contextid_rank_to_vci(ctxid_in_effect, receiver_rank);
        }
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
    bool use_tag = comm->hints[MPIR_COMM_HINT_NO_ANY_TAG];
    bool use_source = comm->hints[MPIR_COMM_HINT_NO_ANY_SOURCE];

    if (is_vci_restricted_to_zero(comm)) {
        vci_idx = 0;
    } else if (use_user_defined_vci) {
        vci_idx = comm->hints[MPIR_COMM_HINT_RECEIVER_VCI] % MPIDI_global.n_vcis;
    } else {
        /* If mpi_any_tag and mpi_any_source can be used for recv, all messages
         * should be received on a single vci. Otherwise, messages sent from a
         * rank can concurrently match at different vcis. This can allow a
         * mesasge to be received in different order than it was sent. We
         * should avoid this.
         * However, if we know mpi_any_source and MPI_any_tag are absent, we
         * don't have this risk and hence we can utilize multiple vcis on the
         * receive side.
         */
        if (use_tag && use_source) {
            vci_idx = MPIDI_map_contextid_rank_tag_to_vci(ctxid_in_effect, sender_rank, tag);
        } else if (use_source) {
            vci_idx = MPIDI_map_contextid_rank_to_vci(ctxid_in_effect, sender_rank);
        } else if (use_tag) {
            vci_idx = MPIDI_map_contextid_tag_to_vci(ctxid_in_effect, tag);
        } else {
            /* General unoptimized case */
            vci_idx = MPIDI_map_contextid_to_vci(ctxid_in_effect);
        }
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
