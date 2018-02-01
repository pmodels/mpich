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

#undef FUNCNAME
#define FUNCNAME MPII_Algo_calculate_pipeline_chunk_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPII_Algo_calculate_pipeline_chunk_info(int hint_in_bytes,
                                                   int type_size, int count,
                                                   int *num_segments,
                                                   int *segsize_floor, int *segsize_ceil)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_ALGO_CALCULATE_PIPELINE_CHUNK_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_ALGO_CALCULATE_PIPELINE_CHUNK_INFO);

    if (count == 0 || type_size == 0) {
        *num_segments = *segsize_floor = *segsize_ceil = 0;
        goto fn_exit;
    }

    if (hint_in_bytes == 0) {    /* no user hint, disable pipelining */
        *num_segments = 1;
        *segsize_floor = *segsize_ceil = count;
        goto fn_exit;
    }

    /* try to see what count we can use that is as close to the user
     * hint as possible, while ensuring that each chunk uses full
     * datatypes without splitting them */
    *segsize_ceil = (hint_in_bytes + type_size - 1) / type_size;
    if (*segsize_ceil > count)
        *segsize_ceil = count;
    *segsize_floor = count % (*segsize_ceil);
    *segsize_floor = *segsize_floor ? *segsize_floor : *segsize_ceil;
    *num_segments = (count + *segsize_ceil - 1) / (*segsize_ceil);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "num_segments %d \n", *num_segments));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_ALGO_CALCULATE_PIPELINE_CHUNK_INFO);

  fn_exit:
    return mpi_errno;
}

#endif /* ALGO_COMMON_H_INCLUDED */
