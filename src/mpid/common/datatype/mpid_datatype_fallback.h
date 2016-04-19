/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIR_DATATYPE_FALLBACK_H_INCLUDED
#define MPIR_DATATYPE_FALLBACK_H_INCLUDED

#include "mpidu_datatype.h"

/* Variables */
#define MPIR_Datatype_direct MPIDU_Datatype_direct
#define MPIR_Datatype_builtin MPIDU_Datatype_builtin
#define MPIR_Datatype_mem MPIDU_Datatype_mem

/* Macros */
#define MPIR_DATATYPE_N_BUILTIN MPIDU_DATATYPE_N_BUILTIN
#define MPID_DTYPE_BEGINNING MPIDU_DTYPE_BEGINNING
#define MPID_DTYPE_END MPIDU_DTYPE_END

typedef MPIDU_Datatype MPIR_Datatype;
typedef MPIDU_Datatype_contents MPIR_Datatype_contents;

#define MPID_Datatype_add_ref MPIDU_Datatype_add_ref
#define MPID_Datatype_committed_ptr MPIDU_Datatype_committed_ptr
#define MPID_Datatype_get_basic_size MPIDU_Datatype_get_basic_size
#define MPID_Datatype_get_basic_type MPIDU_Datatype_get_basic_type

#define MPID_Datatype_get_ptr MPIDU_Datatype_get_ptr
#define MPID_Datatype_get_size_macro MPIDU_Datatype_get_size_macro
#define MPID_Datatype_get_extent_macro MPIDU_Datatype_get_extent_macro
#define MPID_Datatype_is_contig MPIDU_Datatype_is_contig
#define MPID_Datatype_release MPIDU_Datatype_release
#define MPIR_Datatype_valid_ptr MPIDU_Datatype_valid_ptr

#define MPID_Datatype_get_loopdepth_macro MPIDU_Datatype_get_loopdepth_macro
#define MPID_Datatype_get_loopptr_macro MPIDU_Datatype_get_loopptr_macro
#define MPID_Datatype_get_loopsize_macro MPIDU_Datatype_get_loopsize_macro
#define MPID_Datatype_set_loopdepth_macro MPIDU_Datatype_set_loopdepth_macro
#define MPID_Datatype_set_loopptr_macro MPIDU_Datatype_set_loopptr_macro
#define MPID_Datatype_set_loopsize_macro MPIDU_Datatype_set_loopsize_macro

#define MPID_Datatype_free MPIDU_Datatype_free
#define MPID_Datatype_free_contents MPIDU_Datatype_free_contents
#define MPID_Datatype_set_contents MPIDU_Datatype_set_contents
#define MPID_Datatype_size_external32 MPIDU_Datatype_size_external32

/* MPID_Segment */
typedef struct DLOOP_Segment MPID_Segment; /* MPIDU_Segment */

#define MPID_Segment_init MPIDU_Segment_init
#define MPID_Segment_alloc MPIDU_Segment_alloc
#define MPID_Segment_free MPIDU_Segment_free
#define MPID_Segment_pack MPIDU_Segment_pack
#define MPID_Segment_unpack MPIDU_Segment_unpack
#define MPID_Segment_pack_external32 MPIDU_Segment_pack_external32
#define MPID_Segment_unpack_external32 MPIDU_Segment_unpack_external32

/* MPID_Type */
#define MPID_Type_access_contents MPIDU_Type_access_contents
#define MPID_Type_blockindexed MPIDU_Type_blockindexed
#define MPID_Type_commit MPIDU_Type_commit
#define MPID_Type_contiguous MPIDU_Type_contiguous
#define MPID_Type_create_pairtype MPIDU_Type_create_pairtype
#define MPID_Type_create_resized MPIDU_Type_create_resized
#define MPID_Type_dup MPIDU_Type_dup
#define MPID_Type_get_contents MPIDU_Type_get_contents
#define MPID_Type_get_envelope MPIDU_Type_get_envelope
#define MPID_Type_indexed MPIDU_Type_indexed
#define MPID_Type_release_contents MPIDU_Type_release_contents
#define MPID_Type_struct MPIDU_Type_struct
#define MPID_Type_vector MPIDU_Type_vector
#define MPID_Type_zerolen MPIDU_Type_zerolen

#endif /* MPiD_DATATYPE_FALLBACK_H_INCLUDED */
