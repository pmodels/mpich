
subroutine MPI_Type_match_size_f08 (typeclass, size, datatype, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_c_interface, only: c_Datatype, MPIR_Type_match_size_c

    integer,intent(in) :: typeclass
    integer,intent(in) :: size
    type(MPI_Datatype),intent(out) :: datatype
    integer,optional,intent(out) :: ierror

    integer(c_int) :: typeclass_c
    integer(c_int) :: size_c
    integer(c_Datatype) :: datatype_c
    integer(c_int) :: res

    typeclass_c = typeclass
    size_c = size
    res = MPIR_Type_match_size_c (typeclass_c, size_c, datatype_c)
    datatype%MPI_VAL = datatype_c
    if (present(ierror)) ierror = res

end subroutine MPI_Type_match_size_f08


