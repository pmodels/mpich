/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This header provides declarations for MPI functions and types that have
 * been removed from the MPI standard. It is included by mpi.h when the
 * MPI_LEGACY macro is defined (e.g. via mpicc -mpi-legacy).
 *
 * For MPICH ABI builds, these declarations are also available via mpix.h
 * and mpi.h directly. For MPI ABI builds, this header is the only way
 * to access them.
 *
 * New code should use the replacement functions specified by the MPI standard.
 */

#ifndef MPI_LEGACY_H_INCLUDED
#define MPI_LEGACY_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

/* ---- Removed datatype constants ---- */

/* MPI_LB and MPI_UB were removed from the MPI standard. These values are
 * picked from the unassigned slots in the language-independent datatype
 * range (0b0010_00000_***) of the MPI ABI handle encoding and may change
 * in the future to avoid collisions with new standard datatypes.
 */
#define MPI_LB ((MPI_Datatype)0x206)
#define MPI_UB ((MPI_Datatype)0x205)

/* ---- Removed typedefs ---- */

/* Removed in MPI-2.2 (replaced by MPI_Comm_errhandler_function) */
typedef void (MPI_Handler_function) (MPI_Comm *, int *, ...);

/* Removed in MPI-5.0 (replaced by MPI_Comm_copy_attr_function / MPI_Comm_delete_attr_function) */
typedef int (MPI_Copy_function)(MPI_Comm comm, int keyval, void *extra_state,
                                void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPI_Delete_function)(MPI_Comm comm, int keyval,
                                  void *attribute_val, void *extra_state);

/* ---- Removed constants ---- */

/* Removed in MPI-5.0 (replaced by MPI_COMM_NULL_COPY_FN etc.) */
#define MPI_NULL_COPY_FN   ((MPI_Copy_function *)0)
#define MPI_NULL_DELETE_FN ((MPI_Delete_function *)0)
#define MPI_DUP_FN         ((MPI_Copy_function *)0x1)

/* ---- Removed MPI-1 functions (replaced in MPI-2.0) ---- */

int MPI_Address(void *location, MPI_Aint *address);
int MPI_Type_hindexed(int count, int array_of_blocklengths[],
                      MPI_Aint array_of_displacements[],
                      MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_hvector(int count, int blocklength, MPI_Aint stride,
                     MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_struct(int count, int array_of_blocklengths[],
                    MPI_Aint array_of_displacements[],
                    MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent);
int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement);
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement);
int MPI_Errhandler_create(MPI_Comm_errhandler_function *comm_errhandler_fn,
                          MPI_Errhandler *errhandler);
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler);
int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);

/* ---- Removed MPI-1 attribute functions (replaced in MPI-2.0) ---- */

int MPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn,
                      int *keyval, void *extra_state);
int MPI_Keyval_free(int *keyval);
int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int MPI_Attr_delete(MPI_Comm comm, int keyval);

/* ---- PMPI variants ---- */

int PMPI_Address(void *location, MPI_Aint *address);
int PMPI_Type_hindexed(int count, int array_of_blocklengths[],
                       MPI_Aint array_of_displacements[],
                       MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_hvector(int count, int blocklength, MPI_Aint stride,
                      MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_struct(int count, int array_of_blocklengths[],
                     MPI_Aint array_of_displacements[],
                     MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent);
int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement);
int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement);
int PMPI_Errhandler_create(MPI_Comm_errhandler_function *comm_errhandler_fn,
                           MPI_Errhandler *errhandler);
int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler);
int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);
int PMPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn,
                       int *keyval, void *extra_state);
int PMPI_Keyval_free(int *keyval);
int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int PMPI_Attr_delete(MPI_Comm comm, int keyval);

#if defined(__cplusplus)
}
#endif

#endif /* MPI_LEGACY_H_INCLUDED */
