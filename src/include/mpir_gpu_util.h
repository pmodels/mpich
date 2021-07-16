/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_GPU_UTIL_H_INCLUDED
#define MPIR_GPU_UTIL_H_INCLUDED

/* A helper function for allocating temporary buffer */
MPL_STATIC_INLINE_PREFIX void *MPIR_alloc_buffer(MPI_Aint count, MPI_Datatype datatype)
{
    MPI_Aint extent, true_lb, true_extent;
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPI_Aint shift = true_lb;
    MPI_Aint size = true_extent;
    if (count > 1) {
        if (extent >= 0) {
            size += extent * (count - 1);
        } else {
            MPI_Aint size_before = (-extent) * (count - 1);
            /* extra counts are being packed *before*, so need *increase* the size */
            size += size_before;
            /* the allocated pointer will be *already* shifted by this amount */
            shift -= size_before;
        }
    }
    void *host_buf = MPL_malloc(size, MPL_MEM_OTHER);
    MPIR_Assert(host_buf);

    host_buf = (char *) host_buf - shift;
    return host_buf;
}

/* If buf is device memory, allocate a host buffer that can hold the data,
 * return the host buffer, or NULL if buf is not device memory */
MPL_STATIC_INLINE_PREFIX void *MPIR_gpu_host_alloc(const void *buf,
                                                   MPI_Aint count, MPI_Datatype datatype)
{
    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(buf, &attr);

    if (attr.type != MPL_GPU_POINTER_DEV) {
        return NULL;
    } else {
        return MPIR_alloc_buffer(count, datatype);
    }
}

MPL_STATIC_INLINE_PREFIX void MPIR_gpu_host_free(void *host_buf,
                                                 MPI_Aint count, MPI_Datatype datatype)
{
    MPI_Aint extent, true_lb, true_extent;
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPI_Aint shift = true_lb;
    if (count > 1 && extent < 0) {
        MPI_Aint size_before = (-extent) * (count - 1);
        shift -= size_before;
    }
    /* The pointers was previously shifted */
    host_buf = (char *) host_buf + shift;
    MPL_free(host_buf);
}

/* If buf is device memory, allocate a host buffer and copy the content, and return the host
 * buffer. Return NULL if buf is not a device memory */
MPL_STATIC_INLINE_PREFIX void *MPIR_gpu_host_swap(const void *buf,
                                                  MPI_Aint count, MPI_Datatype datatype)
{
    void *host_buf = MPIR_gpu_host_alloc(buf, count, datatype);
    if (host_buf) {
        MPIR_Localcopy(buf, count, datatype, host_buf, count, datatype);
    }

    return host_buf;
}

MPL_STATIC_INLINE_PREFIX void MPIR_gpu_swap_back(void *host_buf, void *gpu_buf,
                                                 MPI_Aint count, MPI_Datatype datatype)
{
    MPIR_Localcopy(host_buf, count, datatype, gpu_buf, count, datatype);
    MPIR_gpu_host_free(host_buf, count, datatype);
}

#endif /* MPIR_GPU_UTIL_H_INCLUDED */
