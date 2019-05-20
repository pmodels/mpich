/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_DATALOOP_H_INCLUDED
#define MPIR_DATALOOP_H_INCLUDED

#include <mpi.h>

/* NOTE: ASSUMING LAST TYPE IS SIGNED */
#define MPIR_SEGMENT_IGNORE_LAST ((MPI_Aint) -1)

typedef struct MPIR_Dataloop MPIR_Dataloop;
typedef struct MPIR_Segment MPIR_Segment;

/* Dataloop functions */
void MPIR_Dataloop_dup(MPIR_Dataloop * old_loop, MPIR_Dataloop ** new_loop_p);

void MPIR_Dataloop_free(MPIR_Dataloop ** dataloop);

void MPIR_Dataloop_create(MPI_Datatype type, MPIR_Dataloop ** dlp_p);

void MPIR_Dataloop_printf(MPI_Datatype type, int depth, int header);
int MPIR_Dataloop_flatten_size(MPIR_Datatype * dtp, int *flattened_dataloop_size);
int MPIR_Dataloop_flatten(MPIR_Datatype * dtp, void *flattened_dataloop);
int MPIR_Dataloop_unflatten(MPIR_Datatype * dtp, void *flattened_dataloop);

/* Segment functions */
MPIR_Segment *MPIR_Segment_alloc(const void *buf, MPI_Aint count, MPI_Datatype handle);
void MPIR_Segment_free(MPIR_Segment * segp);

void MPIR_Segment_count_contig_blocks(MPIR_Segment * segp,
                                      MPI_Aint first, MPI_Aint * lastp, MPI_Aint * countp);

void MPIR_Segment_pack(struct MPIR_Segment *segp,
                       MPI_Aint first, MPI_Aint * lastp, void *streambuf);
void MPIR_Segment_unpack(struct MPIR_Segment *segp,
                         MPI_Aint first, MPI_Aint * lastp, const void *streambuf);

void MPIR_Segment_to_iov(struct MPIR_Segment *segp,
                         MPI_Aint first, MPI_Aint * lastp, MPL_IOV * vector, int *lengthp);

void MPIR_Segment_from_iov(struct MPIR_Segment *segp,
                           MPI_Aint first, MPI_Aint * lastp, MPL_IOV * vector, int *lengthp);

void MPIR_Segment_pack_external32(struct MPIR_Segment *segp,
                                  MPI_Aint first, MPI_Aint * lastp, void *pack_buffer);

void MPIR_Segment_unpack_external32(struct MPIR_Segment *segp,
                                    MPI_Aint first, MPI_Aint * lastp, const void *unpack_buffer);

#endif /* MPIR_DATALOOP_H_INCLUDED */
