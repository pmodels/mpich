/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_STUB_SEND_H_INCLUDED
#define POSIX_EAGER_STUB_SEND_H_INCLUDED

#include "stub_impl.h"

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_payload_limit(void)
{
    MPIR_Assert(0);
    return 0;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_buf_limit(void)
{
    MPIR_Assert(0);
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_send(int grank, MPIDI_POSIX_am_header_t * msg_hdr,
                                                    const void *am_hdr, MPI_Aint am_hdr_sz,
                                                    const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, MPI_Aint offset,
                                                    MPI_Aint * bytes_sent)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* POSIX_EAGER_STUB_TX_H_INCLUDED */
