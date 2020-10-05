/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_TYPEREP_H_INCLUDED
#define MPIR_TYPEREP_H_INCLUDED

#include <mpi.h>

void MPIR_Typerep_init(void);
void MPIR_Typerep_finalize(void);

int MPIR_Typerep_create_vector(int count, int blocklength, int stride, MPI_Datatype oldtype,
                               MPIR_Datatype * newtype);
int MPIR_Typerep_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype);
int MPIR_Typerep_create_contig(int count, MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_dup(MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_indexed_block(int count, int blocklength, const int *array_of_displacements,
                                      MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_hindexed_block(int count, int blocklength,
                                       const MPI_Aint * array_of_displacements,
                                       MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_indexed(int count, const int *array_of_blocklengths,
                                const int *array_of_displacements, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype);
int MPIR_Typerep_create_hindexed(int count, const int *array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                 MPIR_Datatype * newtype);
int MPIR_Typerep_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                MPIR_Datatype * newtype);
int MPIR_Typerep_create_struct(int count, const int *array_of_blocklengths,
                               const MPI_Aint * array_of_displacements,
                               const MPI_Datatype * array_of_types, MPIR_Datatype * newtype);
int MPIR_Typerep_create_subarray(int ndims, const int *array_of_sizes, const int *array_of_subsizes,
                                 const int *array_of_starts, int order, MPI_Datatype oldtype,
                                 MPIR_Datatype * newtype);
int MPIR_Typerep_create_darray(int size, int rank, int ndims, const int *array_of_gsizes,
                               const int *array_of_distribs, const int *array_of_dargs,
                               const int *array_of_psizes, int order, MPI_Datatype oldtype,
                               MPIR_Datatype * newtype);

void MPIR_Typerep_commit(MPI_Datatype type);
void MPIR_Typerep_free(MPIR_Datatype * typeptr);

int MPIR_Typerep_flatten_size(MPIR_Datatype * datatype_ptr, int *flattened_type_size);
int MPIR_Typerep_flatten(MPIR_Datatype * datatype_ptr, void *flattened_type);
int MPIR_Typerep_unflatten(MPIR_Datatype * datatype_ptr, void *flattened_type);

/* byte-based offset routine: do not use; only maintained for backward
 * compatibility with ch3 */
int MPIR_Typerep_to_iov(const void *buf, MPI_Aint count, MPI_Datatype type, MPI_Aint byte_offset,
                        struct iovec *iov, MPI_Aint max_iov_len, MPI_Aint max_iov_bytes,
                        MPI_Aint * actual_iov_len, MPI_Aint * actual_iov_bytes);
/* iov element count based routine */
int MPIR_Typerep_to_iov_offset(const void *buf, MPI_Aint count, MPI_Datatype type,
                               MPI_Aint iov_offset, struct iovec *iov, MPI_Aint max_iov_len,
                               MPI_Aint * actual_iov_len);
int MPIR_Typerep_iov_len(MPI_Aint count, MPI_Datatype type, MPI_Aint max_iov_bytes,
                         MPI_Aint * iov_len);

int MPIR_Typerep_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes);
int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes);
int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize,
                        void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                        MPI_Aint * actual_unpack_bytes);

int MPIR_Typerep_size_external32(MPI_Datatype type);
int MPIR_Typerep_pack_external(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                               void *outbuf, MPI_Aint * actual_pack_bytes);
int MPIR_Typerep_unpack_external(const void *inbuf, void *outbuf, MPI_Aint outcount,
                                 MPI_Datatype datatype, MPI_Aint * actual_unpack_bytes);

void MPIR_Typerep_debug(MPI_Datatype type);

#endif /* MPIR_TYPEREP_H_INCLUDED */
