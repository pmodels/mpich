/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_DATATYPE_H_INCLUDED
#define MPIR_DATATYPE_H_INCLUDED

/* This routine is used to install an attribute free routine for datatypes
   at finalize-time */
void MPII_Datatype_attr_finalize( void );

#define MPIR_DATATYPE_IS_PREDEFINED(type) \
    ((HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) || \
     (type == MPI_FLOAT_INT) || (type == MPI_DOUBLE_INT) || \
     (type == MPI_LONG_INT) || (type == MPI_SHORT_INT) || \
     (type == MPI_LONG_DOUBLE_INT))

int MPIR_Get_elements_x_impl(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *elements);
int MPIR_Status_set_elements_x_impl(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);
void MPIR_Type_get_extent_x_impl(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
void MPIR_Type_get_true_extent_x_impl(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent);
int MPIR_Type_size_x_impl(MPI_Datatype datatype, MPI_Count *size);

void MPIR_Get_count_impl(const MPI_Status *status, MPI_Datatype datatype, int *count);
int MPIR_Type_commit_impl(MPI_Datatype *datatype);
int MPIR_Type_create_struct_impl(int count,
                                 const int array_of_blocklengths[],
                                 const MPI_Aint array_of_displacements[],
                                 const MPI_Datatype array_of_types[],
                                 MPI_Datatype *newtype);
int MPIR_Type_create_indexed_block_impl(int count,
                                        int blocklength,
                                        const int array_of_displacements[],
                                        MPI_Datatype oldtype,
                                        MPI_Datatype *newtype);
int MPIR_Type_create_hindexed_block_impl(int count, int blocklength,
                                         const MPI_Aint array_of_displacements[],
                                         MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPIR_Type_contiguous_impl(int count,
                              MPI_Datatype old_type,
                              MPI_Datatype *new_type_p);
int MPIR_Type_contiguous_x_impl(MPI_Count count,
                              MPI_Datatype old_type,
                              MPI_Datatype *new_type_p);
void MPIR_Type_get_extent_impl(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
void MPIR_Type_get_true_extent_impl(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent);
void MPIR_Type_get_envelope_impl(MPI_Datatype datatype, int *num_integers, int *num_addresses,
                                 int *num_datatypes, int *combiner);
int MPIR_Type_hvector_impl(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype *newtype_p);
int MPIR_Type_indexed_impl(int count, const int blocklens[], const int indices[],
                           MPI_Datatype old_type, MPI_Datatype *newtype);
void MPIR_Type_free_impl(MPI_Datatype *datatype);
int MPIR_Type_vector_impl(int count, int blocklength, int stride, MPI_Datatype old_type, MPI_Datatype *newtype_p);
int MPIR_Type_struct_impl(int count, const int blocklens[], const MPI_Aint indices[], const MPI_Datatype old_types[], MPI_Datatype *newtype);
int MPIR_Pack_impl(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outcount, MPI_Aint *position);
void MPIR_Pack_size_impl(int incount, MPI_Datatype datatype, MPI_Aint *size);
int MPIR_Unpack_impl(const void *inbuf, MPI_Aint insize, MPI_Aint *position,
                     void *outbuf, int outcount, MPI_Datatype datatype);
void MPIR_Type_lb_impl(MPI_Datatype datatype, MPI_Aint *displacement);

#endif /* MPIR_DATATYPE_H_INCLUDED */
