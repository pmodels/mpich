/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_STREAM_H_INCLUDED
#define MPIR_STREAM_H_INCLUDED

/*S
  MPIR_Stream - Structure of an MPIR stream
  S*/

enum MPIR_Stream_type {
    MPIR_STREAM_GENERAL,
    MPIR_STREAM_GPU,
};

struct MPIR_Stream {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    int type;
    union {
        int id;
        MPL_gpu_stream_t gpu_stream;
        void *dummy;            /* to force a minimum pointer alignment, required by handlemem */
    } u;
    int vci;
#ifdef MPID_DEV_STREAM_DECL
     MPID_DEV_STREAM_DECL
#endif
};

extern MPIR_Object_alloc_t MPIR_Stream_mem;
extern MPIR_Stream MPIR_Stream_direct[];

#endif
