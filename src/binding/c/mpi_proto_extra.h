/* *INDENT-OFF* */
/* extra MPI prototypes that are not auto-generated yet */
/* NOTE: this file is for script consumption, no linebreak to simplify the parsing.
 * The generated src/include/mpi_proto.h will have nice line breaks.
 */

/* MPI_DUP_FN is defined to MPIR_Dup_fn and implemented in src/mpi/attr/dup_fn.c */
int MPI_DUP_FN(MPI_Comm oldcomm, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag) MPICH_API_PUBLIC;

int MPI_Status_c2f(const MPI_Status *c_status, MPI_Fint *f_status) MPICH_API_PUBLIC;
int MPI_Status_f2c(const MPI_Fint *f_status, MPI_Status *c_status) MPICH_API_PUBLIC;

/* Fortran 90-related functions.  These routines are available only if
   Fortran 90 support is enabled
*/
int MPI_Type_create_f90_integer(int range, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_f90_real(int precision, int range, MPI_Datatype *newtype) MPICH_API_PUBLIC;
int MPI_Type_create_f90_complex(int precision, int range, MPI_Datatype *newtype) MPICH_API_PUBLIC;
