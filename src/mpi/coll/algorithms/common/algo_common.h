/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef ALGO_COMMON_H_INCLUDED
#define ALGO_COMMON_H_INCLUDED

/* This is a simple function to compare two integers.
 * It is used for sorting list of ranks. */
#undef FUNCNAME
#define FUNCNAME MPII_Algo_compare_int
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Avoid unused function warning in certain configurations */
static int MPII_Algo_compare_int(const void *a, const void *b) ATTRIBUTE((unused));
static int MPII_Algo_compare_int(const void *a, const void *b)
{
    return (*(int *) a - *(int *) b);
}

#undef FUNCNAME
#define FUNCNAME MPII_Algo_calculate_pipeline_chunk_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Avoid unused function warning in certain configurations */
static int MPII_Algo_calculate_pipeline_chunk_info(int maxbytes, int type_size, int count,
                                                   int *num_segments, int *segsize_floor,
                                                   int *segsize_ceil) ATTRIBUTE((unused));
static int MPII_Algo_calculate_pipeline_chunk_info(int maxbytes,
                                                   int type_size, int count,
                                                   int *num_segments,
                                                   int *segsize_floor, int *segsize_ceil)
{
    int maxelems;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_ALGO_CALCULATE_PIPELINE_CHUNK_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_ALGO_CALCULATE_PIPELINE_CHUNK_INFO);

    if (count == 0 || type_size == 0) {
        *num_segments = *segsize_floor = *segsize_ceil = 0;
        goto fn_exit;
    }

    maxelems = maxbytes / type_size;

    if (maxelems == 0 || maxelems >= count) {   /* disable pipelining */
        *num_segments = 1;
        *segsize_floor = *segsize_ceil = count;
        goto fn_exit;
    }

    *segsize_ceil = maxelems;
    *segsize_floor = count % maxelems;
    if (*segsize_floor == 0)
        *segsize_floor = maxelems;
    *num_segments = (count + *segsize_ceil - 1) / (*segsize_ceil);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "num_segments %d", *num_segments));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_ALGO_CALCULATE_PIPELINE_CHUNK_INFO);

  fn_exit:
    return mpi_errno;
}

#endif /* ALGO_COMMON_H_INCLUDED */
