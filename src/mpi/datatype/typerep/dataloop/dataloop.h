/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef DATALOOP_H_INCLUDED
#define DATALOOP_H_INCLUDED

#include <mpi.h>
#include <mpl.h>

struct MPIR_Datatype;

#define MPIR_DATALOOP_GET_LOOPPTR(a,lptr_) do {                         \
        void *ptr;                                                      \
        switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                        \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);      \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);                 \
            lptr_ = ((MPIR_Datatype *)ptr)->typerep.handle;             \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPIR_Datatype *)                                    \
                   MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem)); \
            lptr_ = ((MPIR_Datatype *)ptr)->typerep.handle;             \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            lptr_ = 0;                                                  \
            break;                                                      \
        }                                                               \
    } while (0)

#define MPIR_DATALOOP_SET_LOOPPTR(a,lptr_) do {                         \
        void *ptr;                                                      \
        switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                        \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);      \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);                 \
            ((MPIR_Datatype *)ptr)->typerep.handle = lptr_;             \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPIR_Datatype *)                                    \
                   MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem)); \
            ((MPIR_Datatype *)ptr)->typerep.handle = lptr_;             \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            lptr_ = 0;                                                  \
            break;                                                      \
        }                                                               \
    } while (0)

int MPIR_Dataloop_create_contiguous(MPI_Aint count, MPI_Datatype oldtype, void **dlp_p);
int MPIR_Dataloop_create_vector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                                int strideinbytes, MPI_Datatype oldtype, void **dlp_p);
int MPIR_Dataloop_create_blockindexed(MPI_Aint count, MPI_Aint blklen, const MPI_Aint * disp_array,
                                      int dispinbytes, MPI_Datatype oldtype, void **dlp_p);
/* we bump up the size of the blocklength array because create_struct might use
 * create_indexed in an optimization, and in course of doing so, generate a
 * request of a large blocklength. */
int MPIR_Dataloop_create_indexed(MPI_Aint count, const MPI_Aint * blocklength_array,
                                 const MPI_Aint * displacement_array, int dispinbytes,
                                 MPI_Datatype oldtype, void **dlp_p);
int MPIR_Dataloop_create_struct(MPI_Aint count, const MPI_Aint * blklen_array,
                                const MPI_Aint * disp_array, const MPI_Datatype * oldtype_array,
                                void **dlp_p);

int MPIR_Dataloop_convert_subarray(int ndims, int *array_of_sizes, int *array_of_subsizes,
                                   int *array_of_starts, int order, MPI_Datatype oldtype,
                                   MPI_Datatype * newtype);
int MPIR_Dataloop_convert_darray(int size, int rank, int ndims, int *array_of_gsizes,
                                 int *array_of_distribs, int *array_of_dargs,
                                 int *array_of_psizes, int order, MPI_Datatype oldtype,
                                 MPI_Datatype * newtype);
void MPIR_Dataloop_create_resized(MPI_Datatype oldtype, MPI_Aint extent, void **new_loop_p_);

void MPIR_Dataloop_free(void **dataloop);
void MPIR_Dataloop_dup(void *old_loop, void **new_loop_p);
int MPIR_Dataloop_flatten_size(struct MPIR_Datatype *dtp, int *flattened_dataloop_size);
int MPIR_Dataloop_flatten(struct MPIR_Datatype *dtp, void *flattened_dataloop);
int MPIR_Dataloop_unflatten(struct MPIR_Datatype *dtp, void *flattened_dataloop);
void MPIR_Dataloop_printf(MPI_Datatype type, int depth, int header);

void MPIR_Dataloop_update_contig(void *dataloop, MPI_Aint extent, MPI_Aint typesize);
void MPIR_Dataloop_get_contig(void *dataloop, int *is_contig, MPI_Aint * num_contig);
int MPIR_Dataloop_iov_len(void *dataloop, MPI_Aint * rem_iov_bytes, MPI_Aint * iov_len);
int MPIR_Dataloop_iov(const void *buf, MPI_Aint count, void *dataloop, MPI_Aint extent,
                      MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint iov_len,
                      MPI_Aint * actual_iov_len);

typedef struct MPIR_Segment MPIR_Segment;

/* NOTE: ASSUMING LAST TYPE IS SIGNED */
#define MPIR_SEGMENT_IGNORE_LAST ((MPI_Aint) -1)

MPIR_Segment *MPIR_Segment_alloc(const void *buf, MPI_Aint count, MPI_Datatype handle);
void MPIR_Segment_free(MPIR_Segment * segp);

void MPIR_Segment_pack(MPIR_Segment * segp, MPI_Aint first, MPI_Aint * lastp, void *streambuf);
void MPIR_Segment_unpack(MPIR_Segment * segp,
                         MPI_Aint first, MPI_Aint * lastp, const void *streambuf);

MPI_Aint MPIR_Dataloop_size_external32(MPI_Datatype type);
void MPIR_Segment_pack_external32(MPIR_Segment * segp,
                                  MPI_Aint first, MPI_Aint * lastp, void *pack_buffer);
void MPIR_Segment_unpack_external32(MPIR_Segment * segp,
                                    MPI_Aint first, MPI_Aint * lastp, const void *unpack_buffer);

void MPIR_Segment_to_iov(MPIR_Segment * segp,
                         MPI_Aint first, MPI_Aint * lastp, struct iovec *vector, int *lengthp);
void MPIR_Segment_count_contig_blocks(MPIR_Segment * segp,
                                      MPI_Aint first, MPI_Aint * lastp, MPI_Aint * countp);

#endif /* MPIR_DATALOOP_H_INCLUDED */
