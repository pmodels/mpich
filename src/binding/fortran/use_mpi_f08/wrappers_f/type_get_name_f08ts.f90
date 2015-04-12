!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_get_name_f08(datatype, type_name, resultlen, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_MAX_OBJECT_NAME
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_get_name_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    type(MPI_Datatype), intent(in) :: datatype
    character(len=MPI_MAX_OBJECT_NAME), intent(out) :: type_name
    integer, intent(out) :: resultlen
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    character(kind=c_char) :: type_name_c(MPI_MAX_OBJECT_NAME+1)
    integer(c_int) :: resultlen_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_get_name_c(datatype%MPI_VAL, type_name_c, resultlen)
    else
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Type_get_name_c(datatype_c, type_name_c, resultlen_c)
        resultlen = resultlen_c
    end if

    call MPIR_Fortran_string_c2f(type_name_c, type_name)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_get_name_f08
