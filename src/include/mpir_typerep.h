/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

int MPIR_Typerep_to_iov(const void *buf, MPI_Aint count, MPI_Datatype type, MPI_Aint offset,
                        MPL_IOV * iov, int max_iov_len, MPI_Aint max_iov_bytes,
                        int *actual_iov_len, MPI_Aint * actual_iov_bytes);
int MPIR_Typerep_iov_len(MPI_Aint count, MPI_Datatype type, MPI_Aint offset,
                         MPI_Aint max_iov_bytes, MPI_Aint * iov_len);

int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes);
int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize,
                        void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                        MPI_Aint * actual_unpack_bytes);

int MPIR_Typerep_pack_external(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                               void *outbuf, MPI_Aint * actual_pack_bytes);
int MPIR_Typerep_unpack_external(const void *inbuf, void *outbuf, MPI_Aint outcount,
                                 MPI_Datatype datatype, MPI_Aint * actual_unpack_bytes);

void MPIR_Typerep_debug(MPI_Datatype type);

#endif /* MPIR_DATALOOP_H_INCLUDED */
