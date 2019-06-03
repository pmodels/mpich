/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_DATALOOP_H_INCLUDED
#define MPIR_DATALOOP_H_INCLUDED

#include <mpi.h>

void MPIR_Typerep_create(MPI_Datatype type, void **typerep_p);
void MPIR_Typerep_free(void **typerep_p);
void MPIR_Typerep_dup(void *old_typerep, void **new_typerep_p);

int MPIR_Typerep_flatten_size(MPIR_Datatype * datatype_ptr, int *flattened_type_size);
int MPIR_Typerep_flatten(MPIR_Datatype * datatype_ptr, void *flattened_type);
int MPIR_Typerep_unflatten(MPIR_Datatype * datatype_ptr, void *flattened_type);

int MPIR_Typerep_to_iov(const void *buf, size_t count, MPI_Datatype type, size_t offset,
                        MPL_IOV * iov, int max_iov_len, size_t max_iov_bytes,
                        int *actual_iov_len, size_t * actual_iov_bytes);
int MPIR_Typerep_iov_len(const void *buf, size_t count, MPI_Datatype type, size_t offset,
                         size_t max_iov_bytes, size_t * iov_len);

int MPIR_Typerep_pack(const void *inbuf, size_t incount, MPI_Datatype datatype,
                      size_t inoffset, void *outbuf, size_t max_pack_bytes,
                      size_t * actual_pack_bytes);
int MPIR_Typerep_unpack(const void *inbuf, size_t insize,
                        void *outbuf, size_t outcount, MPI_Datatype datatype, size_t outoffset,
                        size_t * actual_unpack_bytes);

int MPIR_Typerep_pack_external(const void *inbuf, size_t incount, MPI_Datatype datatype,
                               void *outbuf, size_t * actual_pack_bytes);
int MPIR_Typerep_unpack_external(const void *inbuf, void *outbuf, size_t outcount,
                                 MPI_Datatype datatype, size_t * actual_unpack_bytes);

void MPIR_Typerep_debug(MPI_Datatype type);

#endif /* MPIR_DATALOOP_H_INCLUDED */
