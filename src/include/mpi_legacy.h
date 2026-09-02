#ifndef MPI_LEGACY_H_INCLUDED
#define MPI_LEGACY_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

/* Removed MPI types and constants */

#define MPI_Copy_function              MPI_Comm_copy_attr_function
#define MPI_Delete_function            MPI_Comm_delete_attr_function
#define MPI_Handler_function           MPI_Comm_errhandler_function

#define MPI_DUP_FN                     MPI_COMM_DUP_FN
#define MPI_NULL_COPY_FN               MPI_COMM_NULL_COPY_FN
#define MPI_NULL_DELETE_FN             MPI_COMM_NULL_DELETE_FN

#define MPI_COMBINER_HVECTOR_INTEGER   MPI_COMBINER_HVECTOR
#define MPI_COMBINER_HINDEXED_INTEGER  MPI_COMBINER_HINDEXED
#define MPI_COMBINER_STRUCT_INTEGER    MPI_COMBINER_STRUCT


/* Removed MPI functions */

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

static inline int MPI_Type_extent(MPI_Datatype MPI_datatype, MPI_Aint *MPI_extent)
{
  MPI_Aint MPI_lb;
  return MPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_extent);
}

static inline int MPI_Type_lb(MPI_Datatype MPI_datatype, MPI_Aint *MPI_lb)
{
  MPI_Aint MPI_extent;
  return MPI_Type_get_extent(MPI_datatype, MPI_lb, &MPI_extent);
}

static inline int MPI_Type_ub(MPI_Datatype MPI_datatype, MPI_Aint *MPI_ub)
{
  MPI_Aint MPI_lb; int MPI_ierr;
  MPI_ierr = MPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_ub);
  if (MPI_ierr == MPI_SUCCESS && MPI_ub) *MPI_ub += MPI_lb;
  return MPI_ierr;
}

/* Removed PMPI functions */

#define PMPI_Address                   PMPI_Get_Address

#define PMPI_Attr_delete               PMPI_Comm_delete_attr
#define PMPI_Attr_get                  PMPI_Comm_get_attr
#define PMPI_Attr_put                  PMPI_Comm_set_attr

#define PMPI_Errhandler_create         PMPI_Comm_create_Errhandler
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

static inline int PMPI_Type_extent(MPI_Datatype MPI_datatype, MPI_Aint *MPI_extent)
{
  MPI_Aint MPI_lb;
  return PMPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_extent);
}

static inline int PMPI_Type_lb(MPI_Datatype MPI_datatype, MPI_Aint *MPI_lb)
{
  MPI_Aint MPI_extent;
  return PMPI_Type_get_extent(MPI_datatype, MPI_lb, &MPI_extent);
}

static inline int PMPI_Type_ub(MPI_Datatype MPI_datatype, MPI_Aint *MPI_ub)
{
  MPI_Aint MPI_lb; int MPI_ierr;
  MPI_ierr = PMPI_Type_get_extent(MPI_datatype, &MPI_lb, MPI_ub);
  if (MPI_ierr == MPI_SUCCESS && MPI_ub) *MPI_ub += MPI_lb;
  return MPI_ierr;
}


#if defined(__cplusplus)
}
#endif

#endif /* MPI_LEGACY_H_INCLUDED */
