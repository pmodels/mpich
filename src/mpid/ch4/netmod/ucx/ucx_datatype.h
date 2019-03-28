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
#ifdef HAVE_LIBHCOLL
#include "../../../common/hcoll/hcoll.h"
#endif

struct MPIDI_UCX_pack_state {
    void *buffer;
    size_t count;
    MPI_Datatype datatype;
};

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_Start_pack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPIDI_UCX_Start_pack(void *context, const void *buffer, size_t count)
{
    struct MPIDI_UCX_pack_state *state;

    /* Todo: Add error handling */
    state = MPL_malloc(sizeof(struct MPIDI_UCX_pack_state), MPL_MEM_DATATYPE);

    state->buffer = (void *) buffer;
    state->count = count;
    state->datatype = *((MPI_Datatype *) context);

    return (void *) state;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_Start_unpack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPIDI_UCX_Start_unpack(void *context, void *buffer, size_t count)
{
    struct MPIDI_UCX_pack_state *state;

    /* Todo: Add error handling */
    state = MPL_malloc(sizeof(struct MPIDI_UCX_pack_state), MPL_MEM_DATATYPE);

    state->buffer = buffer;
    state->count = count;
    state->datatype = *((MPI_Datatype *) context);

    return (void *) state;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_Packed_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX size_t MPIDI_UCX_Packed_size(void *state)
{
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;
    MPI_Aint packsize;

    MPIR_Pack_size_impl(pack_state->count, pack_state->datatype, &packsize);

    return (size_t) packsize;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_Pack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX size_t MPIDI_UCX_Pack(void *state, size_t offset, void *dest,
                                               size_t max_length)
{
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;
    MPI_Aint actual_pack_bytes;

    MPIR_Pack_impl(pack_state->buffer, pack_state->count, pack_state->datatype, offset,
                   dest, max_length, &actual_pack_bytes);

    return actual_pack_bytes;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_Unpack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX ucs_status_t MPIDI_UCX_Unpack(void *state, size_t offset, const void *src,
                                                       size_t count)
{
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;
    MPI_Aint max_unpack_bytes;
    MPI_Aint actual_unpack_bytes;
    MPI_Aint packsize;

    MPIR_Pack_size_impl(pack_state->count, pack_state->datatype, &packsize);
    max_unpack_bytes = MPL_MIN(packsize, count);

    MPIR_Unpack_impl(src, max_unpack_bytes, pack_state->buffer, pack_state->count,
                     pack_state->datatype, offset, &actual_unpack_bytes);
    if (unlikely(actual_unpack_bytes != max_unpack_bytes)) {
        return UCS_ERR_MESSAGE_TRUNCATED;
    }

    return UCS_OK;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_Finish_pack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_Finish_pack(void *state)
{
    MPIR_Datatype *dt_ptr;
    struct MPIDI_UCX_pack_state *pack_state = (struct MPIDI_UCX_pack_state *) state;
    MPIR_Datatype_get_ptr(pack_state->datatype, dt_ptr);
    MPIR_Datatype_ptr_release(dt_ptr);
    MPL_free(pack_state);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_type_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    if (datatype_p->is_committed && (int) datatype_p->dev.netmod.ucx.ucp_datatype >= 0) {
        ucp_dt_destroy(datatype_p->dev.netmod.ucx.ucp_datatype);
        datatype_p->dev.netmod.ucx.ucp_datatype = -1;
    }
#if HAVE_LIBHCOLL
    hcoll_type_free_hook(datatype_p);
#endif

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_type_commit_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
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
#if HAVE_LIBHCOLL
    hcoll_type_commit_hook(datatype_p);
#endif

    return 0;
}

#endif /* UCX_DATATYPE_H_INCLUDED */
