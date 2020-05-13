/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDIG_GPU_UTIL_H_INCLUDED
#define MPIDIG_GPU_UTIL_H_INCLUDED

/* MPIDIG_gpu_stage_copy_d2h - moving data from device buffer to stage host buffer. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_stage_copy_d2h(const void *sbuf, void *dbuf,
                                                        MPI_Aint count, MPI_Datatype datatype)
{
    size_t size;
    MPIR_Datatype_get_size_macro(datatype, size);

    uintptr_t actual_pack_bytes;
    MPIR_Typerep_pack(sbuf, count, datatype, 0, dbuf, count * size, &actual_pack_bytes);
    MPIR_Assert(actual_pack_bytes == count * size);
}

/* MPIDIG_gpu_stage_copy_h2d - moving data from stage host buffer to device buffer. */
MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_stage_copy_h2d(const void *sbuf, void *dbuf,
                                                        MPI_Aint count, MPI_Datatype datatype)
{
    size_t size;
    MPIR_Datatype_get_size_macro(datatype, size);

    MPI_Aint actual_unpack_bytes;
    MPIR_Typerep_unpack(sbuf, count * size, dbuf, count, datatype, 0, &actual_unpack_bytes);
    MPIR_Assert(actual_unpack_bytes == count * size);
}

#endif /* MPIDIG_GPU_UTIL_H_INCLUDED */
