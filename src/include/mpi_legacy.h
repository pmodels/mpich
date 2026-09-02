#ifndef MPI_LEGACY_H_INCLUDED
#define MPI_LEGACY_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

typedef int (MPI_Copy_function) (MPI_Comm, int, void *, void *, void *, int *);
typedef int (MPI_Delete_function) (MPI_Comm, int, void *, void *);
typedef void (MPI_Handler_function) ( MPI_Comm *, int *, ... );

#define MPI_NULL_COPY_FN   ((MPI_Copy_function*)MPI_COMM_NULL_COPY_FN)
#define MPI_DUP_FN         ((MPI_Copy_function*)MPI_COMM_DUP_FN)
#define MPI_NULL_DELETE_FN ((MPI_Delete_function*)MPI_COMM_NULL_DELETE_FN)

int MPI_Address(void *location, MPI_Aint * address);
int MPI_Type_hindexed(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                      MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                     MPI_Datatype * newtype);
int MPI_Type_struct(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                    MPI_Datatype array_of_types[], MPI_Datatype * newtype);
int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent);
int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint * displacement);
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint * displacement);
int MPI_Errhandler_create(MPI_Comm_errhandler_function * comm_errhandler_fn,
                          MPI_Errhandler * errhandler);
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler * errhandler);
int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);
int MPI_Attr_delete(MPI_Comm comm, int keyval);
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int MPI_Keyval_create(MPI_Copy_function * copy_fn, MPI_Delete_function * delete_fn,
                      int *keyval, void *extra_state);
int MPI_Keyval_free(int *keyval);


int PMPI_Address(void *location, MPI_Aint * address);
int PMPI_Type_hindexed(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                       MPI_Datatype oldtype, MPI_Datatype * newtype);
int PMPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                      MPI_Datatype * newtype);
int PMPI_Type_struct(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
                     MPI_Datatype array_of_types[], MPI_Datatype * newtype);
int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent);
int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * displacement);
int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * displacement);
int PMPI_Errhandler_create(MPI_Comm_errhandler_function * comm_errhandler_fn,
                           MPI_Errhandler * errhandler);
int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler * errhandler);
int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);
int PMPI_Attr_delete(MPI_Comm comm, int keyval);
int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int PMPI_Keyval_create(MPI_Copy_function * copy_fn, MPI_Delete_function * delete_fn,
                       int *keyval, void *extra_state);
int PMPI_Keyval_free(int *keyval);

#if defined(__cplusplus)
}
#endif

#endif /* MPI_LEGACY_H_INCLUDED */
