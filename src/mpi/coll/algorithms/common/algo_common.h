/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef ALGO_COMMON_H_INCLUDED
#define ALGO_COMMON_H_INCLUDED

/* This is a simple function to compare two integers.
 * It is used for sorting list of ranks. */
/* Avoid unused function warning in certain configurations */
static int MPII_Algo_compare_int(const void *a, const void *b) ATTRIBUTE((unused));
static int MPII_Algo_compare_int(const void *a, const void *b)
{
    return (*(int *) a - *(int *) b);
}

/* Avoid unused function warning in certain configurations */
static inline int MPIR_Algo_calculate_pipeline_chunk_info(MPI_Aint chunk_size, MPI_Aint type_size,
                                                          MPI_Aint count, MPI_Aint * num_segments,
                                                          MPI_Aint * segsize_floor,
                                                          MPI_Aint *
                                                          segsize_ceil) ATTRIBUTE((unused));
static inline int MPIR_Algo_calculate_pipeline_chunk_info(MPI_Aint chunk_size, MPI_Aint type_size,
                                                          MPI_Aint count, MPI_Aint * num_segments,
                                                          MPI_Aint * segsize_floor,
                                                          MPI_Aint * segsize_ceil)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (count == 0 || type_size == 0) {
        *num_segments = *segsize_floor = *segsize_ceil = 0;
        goto fn_exit;
    }

    MPI_Aint maxelems;
    maxelems = chunk_size / type_size;

    if (chunk_size <= 0 || maxelems >= count || maxelems < 1) { /* disable pipelining */
        *num_segments = 1;
        *segsize_floor = *segsize_ceil = count;
        goto fn_exit;
    }

    *segsize_ceil = maxelems;
    *segsize_floor = count % maxelems;
    if (*segsize_floor == 0)
        *segsize_floor = maxelems;
    *num_segments = (count + *segsize_ceil - 1) / (*segsize_ceil);

  fn_exit:
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "pipeline info: segsize=" MPI_AINT_FMT_DEC_SPEC
                     " count=" MPI_AINT_FMT_DEC_SPEC
                     " num_chunks=" MPI_AINT_FMT_DEC_SPEC
                     " chunk_count_floor=" MPI_AINT_FMT_DEC_SPEC
                     " chunk_count_ceil=" MPI_AINT_FMT_DEC_SPEC " \n",
                     chunk_size, count, *num_segments, *segsize_floor, *segsize_ceil));

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* ALGO_COMMON_H_INCLUDED */
