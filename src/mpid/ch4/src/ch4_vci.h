/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_VCI_H_INCLUDED
#define CH4_VCI_H_INCLUDED

#include "ch4_impl.h"

/* vci is embedded in the request's pool index */

#define MPIDI_Request_get_vci(req) \
    (((req)->handle & REQUEST_POOL_MASK) >> REQUEST_POOL_SHIFT)

/* VCI hashing function (fast path)
 * Call these function to get src_vci and dst_vci
 * NOTE: The returned vci should always MOD NUMVCIS, where NUMVCIS is
 *       the number of VCIs determined at init time
 *       Potentially, we'd like to make it config constants of power of 2
 * TODO: move the MOD here.
 */

#if MPIDI_CH4_VCI_METHOD == MPICH_VCI__ZERO
/* Always and only use vci 0, equivalent to NUM_VCIS==1 */
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return 0;
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__COMM
/* One communicator per vci. VCI assigned at communitor creation */
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return comm_ptr->seq;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return comm_ptr->seq;
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__TAG
/* A way to allow explicit vci by user, based on (undocumented) convention.
   Embed src_vci and dst_vci inside tag, 5 bits each
   NOTE: this serve as temporary replacement for explicit scheme (until mpi-layer api adds support).
   TODO: ways to allow customization of bit schemes
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return (tag == MPI_ANY_TAG) ? 0 : ((tag >> 10) & 0x1f);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return (tag == MPI_ANY_TAG) ? 0 : ((tag >> 5) & 0x1f);
}

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__IMPLICIT
/* Figure out vci based on (comm, rank, tag) plus hints
 * This is essentially an "auto" method, we use "implicit" as a contrast * to "explicit", which could be available with, e.g. MPI endpoints.
 */
#error "MPICH_VCI__IMPLICIT is to be implemented."
#define MPIDI_vci_get_src MPIDI_vci_get_src_auto
#define MPIDI_vci_get_dst MPIDI_vci_get_dst_auto

#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__EXPLICIT
/* User set explicit src_vci/dst_vci per operation, possibly with
 * MPIX_Send/Recv(..., int src_vci, int dst_vci) or MPI endpoints.
 */
#error "MPICH_VCI__EXPLICIT is to be implemented."
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    MPIR_Assert(0);     /* Should not be called under EXPLICIT scheme */
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    MPIR_Assert(0);     /* Should not be called under EXPLICIT scheme */
    return 0;
}

#else
#error Invalid MPIDI_CH4_VCI_METHOD!
#endif

#endif /* CH4_VCI_H_INCLUDED */
