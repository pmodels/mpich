/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#define GPU_STREAM_USE_SINGLE_VCI

#ifdef GPU_STREAM_USE_SINGLE_VCI
static int gpu_stream_vci = 0;
static int gpu_stream_count = 0;

static int allocate_vci(int *vci, bool is_gpu_stream)
{
    if (is_gpu_stream) {
        int mpi_errno = MPI_SUCCESS;
        if (!gpu_stream_vci) {
            mpi_errno = MPID_Allocate_vci(&gpu_stream_vci, true);       /* shared */
        }
        gpu_stream_count++;
        *vci = gpu_stream_vci;
        return mpi_errno;
    } else {
        return MPID_Allocate_vci(vci, false);   /* not shared */
    }
}

static int deallocate_vci(int vci)
{
    if (vci == gpu_stream_vci) {
        gpu_stream_count--;
        if (gpu_stream_count == 0) {
            gpu_stream_vci = 0;
            return MPID_Deallocate_vci(vci);
        } else {
            return MPI_SUCCESS;
        }
    } else {
        return MPID_Deallocate_vci(vci);
    }
}

#else
static int allocate_vci(int *vci, bool is_gpu_stream)
{
    /* TODO: need make sure the gpu enqueue path is thread-safe */
    return MPID_Allocate_vci(vci, false);
}

static int deallocate_vci(int *vci)
{
    return MPID_Deallocate_vci(vci);
}

#endif

MPIR_Stream MPIR_Stream_direct[MPIR_STREAM_PREALLOC];

MPIR_Object_alloc_t MPIR_Stream_mem = { 0, 0, 0, 0, 0, 0, MPIR_STREAM,
    sizeof(MPIR_Stream), MPIR_Stream_direct,
    MPIR_STREAM_PREALLOC,
    NULL, {0}
};

/* utilities for managing streams in a communicator */

void MPIR_stream_comm_init(MPIR_Comm * comm)
{
    comm->stream_comm_type = MPIR_STREAM_COMM_NONE;
}

void MPIR_stream_comm_free(MPIR_Comm * comm)
{
    if (comm->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        if (comm->stream_comm.single.stream) {
            MPIR_Stream_free_impl(comm->stream_comm.single.stream);
        }
        MPL_free(comm->stream_comm.single.vci_table);
    } else if (comm->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        int rank = comm->rank;
        int num_local_streams = comm->stream_comm.multiplex.vci_displs[rank + 1] -
            comm->stream_comm.multiplex.vci_displs[rank];
        for (int i = 0; i < num_local_streams; i++) {
            if (comm->stream_comm.multiplex.local_streams[i]) {
                MPIR_Stream_free_impl(comm->stream_comm.multiplex.local_streams[i]);
            }
        }
        MPL_free(comm->stream_comm.multiplex.local_streams);
        MPL_free(comm->stream_comm.multiplex.vci_displs);
        MPL_free(comm->stream_comm.multiplex.vci_table);
    }
}

int MPIR_Comm_copy_stream(MPIR_Comm * oldcomm, MPIR_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;

    newcomm->stream_comm_type = oldcomm->stream_comm_type;
    if (oldcomm->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        MPIR_Stream *stream_ptr = oldcomm->stream_comm.single.stream;

        int *vci_table;
        vci_table = MPL_malloc(oldcomm->local_size * sizeof(int), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!vci_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

        for (int i = 0; i < oldcomm->local_size; i++) {
            vci_table[i] = oldcomm->stream_comm.single.vci_table[i];
        }

        newcomm->stream_comm.single.stream = stream_ptr;
        newcomm->stream_comm.single.vci_table = vci_table;

        if (stream_ptr) {
            MPIR_Object_add_ref(stream_ptr);
        }
    } else if (oldcomm->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        int size = oldcomm->local_size;
        int rank = oldcomm->rank;

        MPI_Aint *displs;
        /* note: we allocate (size + 1) so the counts can be calculated from displs table */
        displs = MPL_malloc((size + 1) * sizeof(MPI_Aint), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!displs, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 0; i < size + 1; i++) {
            displs[i] = oldcomm->stream_comm.multiplex.vci_displs[i];
        }

        int num_total = displs[size];
        int num_streams = displs[rank + 1] - displs[rank];

        int *vci_table;
        vci_table = MPL_malloc(num_total * sizeof(int), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!vci_table, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 0; i < num_total; i++) {
            vci_table[i] = oldcomm->stream_comm.multiplex.vci_table[i];
        }

        MPIR_Stream **local_streams;
        local_streams = MPL_malloc(num_streams * sizeof(MPIR_Stream *), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!local_streams, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 0; i < num_streams; i++) {
            local_streams[i] = oldcomm->stream_comm.multiplex.local_streams[i];
            if (local_streams[i]) {
                MPIR_Object_add_ref(local_streams[i]);
            }
        }

        newcomm->stream_comm.multiplex.local_streams = local_streams;
        newcomm->stream_comm.multiplex.vci_displs = displs;
        newcomm->stream_comm.multiplex.vci_table = vci_table;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Impl routines */

int MPIR_Stream_create_impl(MPIR_Info * info_ptr, MPIR_Stream ** p_stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Stream *stream_ptr;
    stream_ptr = (MPIR_Stream *) MPIR_Handle_obj_alloc(&MPIR_Stream_mem);
    MPIR_ERR_CHKANDJUMP1(!stream_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "MPI_Stream");

    MPIR_Object_set_ref(stream_ptr, 1);
    stream_ptr->vci = 0;
#ifdef MPID_DEV_STREAM_DECL
    memset(&stream_ptr->dev, 0, sizeof(stream_ptr->dev));
#endif

    const char *s_type;
    s_type = MPIR_Info_lookup(info_ptr, "type");

    if (s_type && strcmp(s_type, "cudaStream_t") == 0) {
#ifndef MPL_HAVE_CUDA
        MPIR_Assert(0 && "CUDA not enabled");
#endif
        stream_ptr->type = MPIR_STREAM_GPU;
    } else if (s_type && strcmp(s_type, "hipStream_t") == 0) {
#ifndef MPL_HAVE_HIP
        MPIR_Assert(0 && "HIP not enabled");
#endif
        stream_ptr->type = MPIR_STREAM_GPU;
    } else {
        stream_ptr->type = MPIR_STREAM_GENERAL;
    }

    if (stream_ptr->type == MPIR_STREAM_GPU) {
        /* TODO: proper conversion for each gpu stream type */
        const char *s_value = MPIR_Info_lookup(info_ptr, "value");
        MPIR_ERR_CHKANDJUMP(!s_value, mpi_errno, MPI_ERR_OTHER, "**missinggpustream");

        mpi_errno =
            MPIR_Info_decode_hex(s_value, &stream_ptr->u.gpu_stream, sizeof(MPL_gpu_stream_t));
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(!MPL_gpu_stream_is_valid(stream_ptr->u.gpu_stream),
                            mpi_errno, MPI_ERR_OTHER, "**invalidgpustream");
    }

    mpi_errno = allocate_vci(&stream_ptr->vci, stream_ptr->type == MPIR_STREAM_GPU);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPID_Stream_create_hook(stream_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *p_stream_ptr = stream_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    if (stream_ptr) {
        MPIR_Stream_free_impl(stream_ptr);
    }
    goto fn_exit;
}

int MPIR_Stream_free_impl(MPIR_Stream * stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    int ref_cnt;
    MPIR_Object_release_ref(stream_ptr, &ref_cnt);
    if (ref_cnt == 0) {
        mpi_errno = MPID_Stream_free_hook(stream_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        if (stream_ptr->vci) {
            mpi_errno = deallocate_vci(stream_ptr->vci);
        }
        MPIR_Handle_obj_free(&MPIR_Stream_mem, stream_ptr);
    } else {
        /* The stream is still in use */
        if (stream_ptr->type == MPIR_STREAM_GPU) {
            /* We allow asynchronous free of GPU stream because we reuse a single
             * gpu vci. Nothing to do here. */
        } else {
            /* We need ensure unique vci usage per stream, thus we need warn user
             * when stream is freed while still in-use */
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**cannotfreestream");
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Stream_comm_create_impl(MPIR_Comm * comm_ptr, MPIR_Stream * stream_ptr,
                                 MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Comm_dup(comm_ptr, NULL, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    int vci, *vci_table;
    if (stream_ptr) {
        vci = stream_ptr->vci;
    } else {
        vci = 0;
    }
    vci_table = MPL_malloc(comm_ptr->local_size * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!vci_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno =
        MPIR_Allgather_impl(&vci, 1, MPI_INT, vci_table, 1, MPI_INT, comm_ptr, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcomm_ptr)->stream_comm_type = MPIR_STREAM_COMM_SINGLE;
    (*newcomm_ptr)->stream_comm.single.stream = stream_ptr;
    (*newcomm_ptr)->stream_comm.single.vci_table = vci_table;

    if (stream_ptr) {
        MPIR_Object_add_ref(stream_ptr);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Stream_comm_create_multiplex_impl(MPIR_Comm * comm_ptr,
                                           int num_streams, MPIX_Stream streams[],
                                           MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* if user offers 0 streams, use default MPIX_STREAM_NULL */
    MPIX_Stream stream_default = MPIX_STREAM_NULL;
    if (num_streams == 0) {
        num_streams = 1;
        streams = &stream_default;
    }

    mpi_errno = MPII_Comm_dup(comm_ptr, NULL, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint *num_table;
    num_table = MPL_malloc(comm_ptr->local_size * sizeof(MPI_Aint), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!num_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPI_Aint *displs;
    /* note: we allocate (size + 1) so the counts can be calculated from displs table */
    displs = MPL_malloc((comm_ptr->local_size + 1) * sizeof(MPI_Aint), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!displs, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPI_Aint num_tmp = num_streams;
    mpi_errno = MPIR_Allgather_impl(&num_tmp, 1, MPI_AINT,
                                    num_table, 1, MPI_AINT, comm_ptr, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint num_total = 0;
    for (int i = 0; i < comm_ptr->local_size; i++) {
        displs[i] = num_total;
        num_total += num_table[i];
    }
    displs[comm_ptr->local_size] = num_total;

    int *vci_table;
    vci_table = MPL_malloc(num_total * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!vci_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Stream **local_streams;
    local_streams = MPL_malloc(num_streams * sizeof(MPIR_Stream *), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!local_streams, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int *local_vcis;
    local_vcis = MPL_malloc(num_streams * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!local_vcis, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (int i = 0; i < num_streams; i++) {
        MPIR_Stream *stream_ptr;
        MPIR_Stream_get_ptr(streams[i], stream_ptr);
        if (stream_ptr) {
            MPIR_Object_add_ref(stream_ptr);
        }
        local_streams[i] = stream_ptr;
        local_vcis[i] = stream_ptr ? stream_ptr->vci : 0;
    }

    mpi_errno = MPIR_Allgatherv_impl(local_vcis, num_streams, MPI_INT,
                                     vci_table, num_table, displs, MPI_INT, comm_ptr,
                                     MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcomm_ptr)->stream_comm_type = MPIR_STREAM_COMM_MULTIPLEX;
    (*newcomm_ptr)->stream_comm.multiplex.local_streams = local_streams;
    (*newcomm_ptr)->stream_comm.multiplex.vci_displs = displs;
    (*newcomm_ptr)->stream_comm.multiplex.vci_table = vci_table;

    MPL_free(local_vcis);
    MPL_free(num_table);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_get_stream_impl(MPIR_Comm * comm_ptr, int idx, MPIR_Stream ** stream_out)
{
    int mpi_errno = MPI_SUCCESS;

    *stream_out = NULL;
    if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        if (idx == 0) {
            *stream_out = comm_ptr->stream_comm.single.stream;
        }
    } else if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        int rank = comm_ptr->rank;
        MPI_Aint *displs = comm_ptr->stream_comm.multiplex.vci_displs;
        int num_streams = displs[rank + 1] - displs[rank];
        if (idx >= 0 && idx < num_streams) {
            *stream_out = comm_ptr->stream_comm.multiplex.local_streams[displs[rank] + idx];
        }
    }

    return mpi_errno;
}
