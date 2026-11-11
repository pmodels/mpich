/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This header provides the MPI functions and types that have been removed
 * from the MPI standard, for source compatibility with old codes. New code
 * should use the replacement functions specified by the MPI standard.
 *
 * MPI ABI: the MPI ABI mpi.h includes this header when MPI_LEGACY is defined
 * (e.g. via mpicc -mpi-legacy). The removed APIs are mapped to their
 * replacements with macros and static inline functions, so libmpi_abi does
 * not export any removed symbol and the program stays portable across MPI ABI
 * implementations. MPI_LB and MPI_UB are not provided.
 *
 * MPICH native ABI: the removed APIs are always available. They are real
 * functions in libmpi for backward compatibility, declared in mpi.h and
 * mpi_proto.h.
 */

#ifndef MPI_LEGACY_H_INCLUDED
#define MPI_LEGACY_H_INCLUDED

#ifdef MPI_ABI_VERSION

#if defined(__cplusplus)
extern "C" {
#endif

/* ---- Removed types and constants ---- */

#define MPI_Copy_function              MPI_Comm_copy_attr_function
#define MPI_Delete_function            MPI_Comm_delete_attr_function
#define MPI_Handler_function           MPI_Comm_errhandler_function

#define MPI_DUP_FN                     MPI_COMM_DUP_FN
#define MPI_NULL_COPY_FN               MPI_COMM_NULL_COPY_FN
#define MPI_NULL_DELETE_FN             MPI_COMM_NULL_DELETE_FN

/* Defined to unique values so legacy switch cases can compile */
#define MPI_COMBINER_HVECTOR_INTEGER   (MPI_COMBINER_NAMED-1)
#define MPI_COMBINER_HINDEXED_INTEGER  (MPI_COMBINER_NAMED-2)
#define MPI_COMBINER_STRUCT_INTEGER    (MPI_COMBINER_NAMED-3)

/* MPI_LB and MPI_UB are not provided, so using them is a compile-time
 * error. They are markers that only work if the MPI library interprets
 * them, which a standard libmpi_abi need not do. Use MPI_Type_create_resized
 * to set the lower bound and extent of a datatype instead.
 */

/* ---- Removed functions ---- */

#define MPI_Address                    MPI_Get_address

#define MPI_Attr_delete                MPI_Comm_delete_attr
#define MPI_Attr_get                   MPI_Comm_get_attr
#define MPI_Attr_put                   MPI_Comm_set_attr

#define MPI_Errhandler_create          MPI_Comm_create_errhandler
#define MPI_Errhandler_get             MPI_Comm_get_errhandler
#define MPI_Errhandler_set             MPI_Comm_set_errhandler

#define MPI_Keyval_create              MPI_Comm_create_keyval
#define MPI_Keyval_free                MPI_Comm_free_keyval

#define MPI_Type_hindexed              MPI_Type_create_hindexed
#define MPI_Type_hvector               MPI_Type_create_hvector
#define MPI_Type_struct                MPI_Type_create_struct

#define MPI_Type_extent                MPI_ABI_Type_extent
#define MPI_Type_lb                    MPI_ABI_Type_lb
#define MPI_Type_ub                    MPI_ABI_Type_ub

static inline int MPI_Type_extent(MPI_Datatype MPI_datatype, MPI_Aint * MPI_extent)
{
    MPI_Aint MPI_lb;
    return MPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_extent);
}

static inline int MPI_Type_lb(MPI_Datatype MPI_datatype, MPI_Aint * MPI_lb)
{
    MPI_Aint MPI_extent;
    return MPI_Type_get_extent(MPI_datatype, MPI_lb, &MPI_extent);
}

static inline int MPI_Type_ub(MPI_Datatype MPI_datatype, MPI_Aint * MPI_ub)
{
    MPI_Aint MPI_lb;
    int MPI_ierr;
    MPI_ierr = MPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_ub);
    if (MPI_ierr == MPI_SUCCESS && MPI_ub)
        *MPI_ub += MPI_lb;
    return MPI_ierr;
}

/* ---- Removed PMPI functions ---- */

#define PMPI_Address                   PMPI_Get_address

#define PMPI_Attr_delete               PMPI_Comm_delete_attr
#define PMPI_Attr_get                  PMPI_Comm_get_attr
#define PMPI_Attr_put                  PMPI_Comm_set_attr

#define PMPI_Errhandler_create         PMPI_Comm_create_errhandler
#define PMPI_Errhandler_get            PMPI_Comm_get_errhandler
#define PMPI_Errhandler_set            PMPI_Comm_set_errhandler

#define PMPI_Keyval_create             PMPI_Comm_create_keyval
#define PMPI_Keyval_free               PMPI_Comm_free_keyval

#define PMPI_Type_hindexed             PMPI_Type_create_hindexed
#define PMPI_Type_hvector              PMPI_Type_create_hvector
#define PMPI_Type_struct               PMPI_Type_create_struct

#define PMPI_Type_extent               PMPI_ABI_Type_extent
#define PMPI_Type_lb                   PMPI_ABI_Type_lb
#define PMPI_Type_ub                   PMPI_ABI_Type_ub

static inline int PMPI_Type_extent(MPI_Datatype MPI_datatype, MPI_Aint * MPI_extent)
{
    MPI_Aint MPI_lb;
    return PMPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_extent);
}

static inline int PMPI_Type_lb(MPI_Datatype MPI_datatype, MPI_Aint * MPI_lb)
{
    MPI_Aint MPI_extent;
    return PMPI_Type_get_extent(MPI_datatype, MPI_lb, &MPI_extent);
}

static inline int PMPI_Type_ub(MPI_Datatype MPI_datatype, MPI_Aint * MPI_ub)
{
    MPI_Aint MPI_lb;
    int MPI_ierr;
    MPI_ierr = PMPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_ub);
    if (MPI_ierr == MPI_SUCCESS && MPI_ub)
        *MPI_ub += MPI_lb;
    return MPI_ierr;
}

#if defined(__cplusplus)
}
#endif

#else
/* MPICH native ABI: the removed APIs are declared in mpi.h and mpi_proto.h */
#endif /* MPI_ABI_VERSION */

#endif /* MPI_LEGACY_H_INCLUDED */
