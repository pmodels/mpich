/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MPIR_ALGO_UTIL_H_INCLUDED
#define MPIR_ALGO_UTIL_H_INCLUDED

#undef FUNCNAME
#define FUNCNAME MPIC_calculate_chunk_info
MPL_STATIC_INLINE_PREFIX int MPIC_calculate_chunk_info(int segsize_input, int type_size, int count,
                                          int *num_chunks, int *num_chunks_floor,
                                          int *chunk_size_floor, int *chunk_size_ceil)
{
    int segsize_bytes, segsize;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_CALCULATE_CHUNK_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_CALCULATE_CHUNK_INFO);

    if (count == 0) {
        *num_chunks = *num_chunks_floor = *chunk_size_floor = *chunk_size_ceil = 0;
        return 0;
    }

    /* check if segsize is given and if it is less than the total size of the data */
    segsize_bytes = (segsize_input == -1) ? count * type_size : segsize_input;

    /* Calculate the #elements for the given segsize closest to the multiple of type_size */
    segsize = 0;
    if (type_size != 0)
        segsize = (segsize_bytes + type_size - 1) / (type_size); /* ceil of segsize_bytes/type_size */

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"segment size (in number of elements)=%d\n", segsize));
    if (segsize != 0)
        *num_chunks = (count + segsize - 1) / segsize;  /*ceil of count/segment_size */
    if (segsize == 0 || *num_chunks == 0)
        *num_chunks = 1;        /* *num_chunks cannot be zero */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"num_chunks %d \n", *num_chunks));

    /* The message is divided into num_chunks */
    *chunk_size_floor = (count) / (*num_chunks); /* smaller of the chunk sizes obtained by integer division */

    /* calculate larger of the chunk size */
    if ((count) % (*num_chunks) == 0)
        *chunk_size_ceil = (*chunk_size_floor); /* all chunk sizes are equal */
    else
        *chunk_size_ceil = (*chunk_size_floor) + 1;

    /* number of chunks of size chunk_size_floor */
    *num_chunks_floor = (*num_chunks) * (*chunk_size_ceil) - count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_CALCULATE_CHUNK_INFO);

    return 0;
}

#endif
