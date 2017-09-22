/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_DATATYPE_H_INCLUDED
#define UCX_DATATYPE_H_INCLUDED

#include "ucx_impl.h"
#include "ucx_types.h"
#include <ucp/api/ucp.h>

struct MPIDI_UCX_pack_state {
    MPIR_Segment *segment_ptr;
    MPI_Aint packsize;
};

static inline void *MPIDI_UCX_Start_pack(void *context, const void *buffer, size_t count)
{
    MPI_Datatype *datatype = (MPI_Datatype *) context;
    MPIR_Segment *segment_ptr;
    struct MPIDI_UCX_pack_state *state;
    MPI_Aint packsize;

    state = MPL_malloc(sizeof(struct MPIDI_UCX_pack_state));
    segment_ptr = MPIR_Segment_alloc();
    MPIR_Pack_size_impl(count, *datatype, &packsize);
    /* Todo: Add error handling */
    MPIR_Segment_init(buffer, count, *datatype, segment_ptr, 1);
    state->packsize = packsize;
    state->segment_ptr = segment_ptr;

    return (void *) state;
}

static inline void *MPIDI_UCX_Start_unpack(void *context, void *buffer, size_t count)
{
    MPI_Datatype *datatype = (MPI_Datatype *) context;
    MPIR_Segment *segment_ptr;
    struct MPIDI_UCX_pack_state *state;
    MPI_Aint packsize;

    state = MPL_malloc(sizeof(struct MPIDI_UCX_pack_state));
    MPIR_Pack_size_impl(count, *datatype, &packsize);
    segment_ptr = MPIR_Segment_alloc();
    /* Todo: Add error handling */
    MPIR_Segment_init(buffer, count, *datatype, segment_ptr, 1);
    state->packsize = packsize;
    state->segment_ptr = segment_ptr;

    return (void *) state;
}

static inline size_t MPIDI_UCX_Packed_size(void *state)
{
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;

    return (size_t) pack_state->packsize;
}

static inline size_t MPIDI_UCX_Pack(void *state, size_t offset, void *dest, size_t max_length)
{
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;
    MPI_Aint last = MPL_MIN(pack_state->packsize, offset + max_length);

    MPIR_Segment_pack(pack_state->segment_ptr, offset, &last, dest);

    return (size_t) last - offset;
}

static inline ucs_status_t MPIDI_UCX_Unpack(void *state, size_t offset, const void *src,
                                            size_t count)
{
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;
    size_t last = MPL_MIN(pack_state->packsize, offset + count);
    size_t last_pack = last;

    MPIR_Segment_unpack(pack_state->segment_ptr, offset, &last, (void *) src);
    if (unlikely(last != last_pack)) {
        return UCS_ERR_MESSAGE_TRUNCATED;
    }

    return UCS_OK;
}

static inline void MPIDI_UCX_Finish_pack(void *state)
{
    MPIR_Datatype *dt_ptr;
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;
    MPIR_Datatype_get_ptr(pack_state->segment_ptr->handle, dt_ptr);
    MPIR_Segment_free(pack_state->segment_ptr);
    MPIR_Datatype_release(dt_ptr);
    MPL_free(pack_state);
}

static inline int MPIDI_NM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    if (datatype_p->is_committed && (int) datatype_p->dev.netmod.ucx.ucp_datatype >= 0) {
        ucp_dt_destroy(datatype_p->dev.netmod.ucx.ucp_datatype);
        datatype_p->dev.netmod.ucx.ucp_datatype = -1;
    }

    return 0;
}

static inline int MPIDI_NM_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    ucp_datatype_t ucp_datatype;
    ucs_status_t status;
    int is_contig;

    datatype_p->dev.netmod.ucx.ucp_datatype = -1;
    MPIR_Datatype_is_contig(datatype_p->handle, &is_contig);

    if (!is_contig) {

        ucp_generic_dt_ops_t dt_ops = {
            .start_pack = MPIDI_UCX_Start_pack,
            .start_unpack = MPIDI_UCX_Start_unpack,
            .packed_size = MPIDI_UCX_Packed_size,
            .pack = MPIDI_UCX_Pack,
            .unpack = MPIDI_UCX_Unpack,
            .finish = MPIDI_UCX_Finish_pack
        };
        status = ucp_dt_create_generic(&dt_ops, datatype_p, &ucp_datatype);
        MPIR_Assertp(status == UCS_OK);
        datatype_p->dev.netmod.ucx.ucp_datatype = ucp_datatype;

    }

    return 0;
}

#endif /* UCX_DATATYPE_H_INCLUDED */
