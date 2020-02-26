/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDIG_GPU_UTIL_H_INCLUDED
#define MPIDIG_GPU_UTIL_H_INCLUDED

#include "ch4_impl.h"

/* MPIDIG_gpu_stage_copy_d2h - moving data from device buffer to stage host buffer. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_stage_copy_d2h(const void *sbuf, void *dbuf,
                                                        MPI_Aint count, MPI_Datatype datatype)
{
    int dt_contig;
    size_t size;
    MPIR_Datatype *dt_ptr;
    uintptr_t actual_pack_bytes;

    MPIDI_Datatype_get_size_dt_ptr(count, datatype, size, dt_ptr);
    dt_contig = (dt_ptr) ? (dt_ptr)->is_contig : 1;
    if (dt_contig) {
        /* Stage buffer is contiguous, we can move directly for contiguous datatype. */
        MPIR_Typerep_pack(sbuf, count, datatype, 0, dbuf, size, &actual_pack_bytes);
    } else {
        double density = ((double) (dt_ptr->size)) / ((double) (dt_ptr->max_contig_blocks));
        if (density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
            /* No need to pack for high-density datatype */
            /* We convert it to iov, then copy each iov into stage buffer */
            int i;
            MPI_Aint curr_len;
            void *curr_loc;
            MPI_Aint num_iov;
            int n_iov;
            struct iovec *dt_iov;
            int actual_iov_len;
            MPI_Aint actual_iov_bytes;
            size_t accumulated_count = 0;

            MPIR_Typerep_iov_len(count, datatype, size, &num_iov);
            n_iov = (int) num_iov;
            dt_iov = (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_BUFFER);
            MPIR_Typerep_to_iov(NULL, count, datatype, 0, dt_iov, n_iov, size, &actual_iov_len,
                                &actual_iov_bytes);
            n_iov = actual_iov_len;
            MPIR_Assert(actual_iov_bytes == (MPI_Aint) size);

            for (i = 0; i < n_iov; i++) {
                curr_loc = dt_iov[i].iov_base;
                curr_len = dt_iov[i].iov_len;
                MPIR_Typerep_pack((char *) sbuf + MPIR_Ptr_to_aint(curr_loc), curr_len, MPI_BYTE, 0,
                                  (char *) dbuf + accumulated_count, curr_len, &actual_pack_bytes);
                accumulated_count += curr_len;
            }
            MPL_free(dt_iov);
        } else {
            /* For low-density datatype, pack into contiguous stage buffer. */
            MPIR_Typerep_pack(sbuf, count, datatype, 0, dbuf, size, &actual_pack_bytes);
        }
    }
}

/* MPIDIG_gpu_stage_copy_h2d - moving data from stage host buffer to device buffer. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_stage_copy_h2d(const void *sbuf, void *dbuf,
                                                        MPI_Aint count, MPI_Datatype datatype)
{
    int dt_contig;
    size_t size;
    MPIR_Datatype *dt_ptr;
    MPI_Aint actual_unpack_bytes;

    MPIDI_Datatype_get_size_dt_ptr(count, datatype, size, dt_ptr);
    dt_contig = (dt_ptr) ? (dt_ptr)->is_contig : 1;
    if (dt_contig) {
        /* Stage buffer is contiguous, we can move directly for contiguous datatype. */
        MPIR_Typerep_unpack(sbuf, size, dbuf, count, datatype, 0, &actual_unpack_bytes);
    } else {
        double density = ((double) (dt_ptr->size)) / ((double) (dt_ptr->max_contig_blocks));
        if (density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
            /* No need to pack for high-density datatype */
            /* We convert it to iov, then copy each iov out of stage buffer */
            int i;
            MPI_Aint curr_len;
            void *curr_loc;
            MPI_Aint num_iov;
            int n_iov;
            struct iovec *dt_iov;
            int actual_iov_len;
            MPI_Aint actual_iov_bytes;
            size_t accumulated_count = 0;

            MPIR_Typerep_iov_len(count, datatype, size, &num_iov);
            n_iov = (int) num_iov;
            dt_iov = (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec), MPL_MEM_BUFFER);
            MPIR_Typerep_to_iov(NULL, count, datatype, 0, dt_iov, n_iov, size, &actual_iov_len,
                                &actual_iov_bytes);
            n_iov = actual_iov_len;
            MPIR_Assert(actual_iov_bytes == (MPI_Aint) size);

            for (i = 0; i < n_iov; i++) {
                curr_loc = dt_iov[i].iov_base;
                curr_len = dt_iov[i].iov_len;
                MPIR_Typerep_unpack((char *) sbuf + accumulated_count, curr_len,
                                    (char *) dbuf + MPIR_Ptr_to_aint(curr_loc), curr_len, MPI_BYTE,
                                    0, &actual_unpack_bytes);
                accumulated_count += curr_len;
            }
            MPL_free(dt_iov);
        } else {
            /* For low-density datatype, unpack out of contiguous stage buffer. */
            MPIR_Typerep_unpack(sbuf, size, dbuf, count, datatype, 0, &actual_unpack_bytes);
        }
    }
}

#endif /* MPIDIG_GPU_UTIL_H_INCLUDED */
