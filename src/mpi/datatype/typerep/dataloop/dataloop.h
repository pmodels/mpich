/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DATALOOP_H_INCLUDED
#define DATALOOP_H_INCLUDED

#include <mpi.h>
#include <mpl.h>

struct MPIR_Datatype;

void MPIR_Dataloop_create(MPI_Datatype type, void **dlp_p);
void MPIR_Dataloop_free(void **dataloop);
void MPIR_Dataloop_dup(void *old_loop, void **new_loop_p);
int MPIR_Dataloop_flatten_size(struct MPIR_Datatype *dtp, int *flattened_dataloop_size);
int MPIR_Dataloop_flatten(struct MPIR_Datatype *dtp, void *flattened_dataloop);
int MPIR_Dataloop_unflatten(struct MPIR_Datatype *dtp, void *flattened_dataloop);
void MPIR_Dataloop_printf(MPI_Datatype type, int depth, int header);

typedef struct MPIR_Segment MPIR_Segment;

/* NOTE: ASSUMING LAST TYPE IS SIGNED */
#define MPIR_SEGMENT_IGNORE_LAST ((size_t) -1)

MPIR_Segment *MPIR_Segment_alloc(const void *buf, size_t count, MPI_Datatype handle);
void MPIR_Segment_free(MPIR_Segment * segp);

void MPIR_Segment_pack(MPIR_Segment * segp, size_t first, size_t * lastp, void *streambuf);
void MPIR_Segment_unpack(MPIR_Segment * segp,
                         size_t first, size_t * lastp, const void *streambuf);

void MPIR_Segment_pack_external32(MPIR_Segment * segp,
                                  size_t first, size_t * lastp, void *pack_buffer);
void MPIR_Segment_unpack_external32(MPIR_Segment * segp,
                                    size_t first, size_t * lastp, const void *unpack_buffer);

void MPIR_Segment_to_iov(MPIR_Segment * segp,
                         size_t first, size_t * lastp, MPL_IOV * vector, int *lengthp);
void MPIR_Segment_count_contig_blocks(MPIR_Segment * segp,
                                      size_t first, size_t * lastp, size_t * countp);

#endif /* MPIR_DATALOOP_H_INCLUDED */
