
subroutine MPI_Type_create_f90_real_f08 (p, r, newtype, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_c_interface, only: c_Datatype, MPIR_Type_create_f90_real_c

    integer,intent(in) :: p
    integer,intent(in) :: r
    type(MPI_Datatype),intent(out) :: newtype
    integer,optional, intent(out) :: ierror

    integer(c_int) :: p_c
    integer(c_int) :: r_c
    integer(c_Datatype) :: newtype_c
    integer(c_int) :: res

    p_c = p
    r_c = r
    res = MPIR_Type_create_f90_real_c (p_c, r_c, newtype_c)
    newtype%MPI_VAL = newtype_c
    if (present(ierror)) ierror = res

end subroutine MPI_Type_create_f90_real_f08


