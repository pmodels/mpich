!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_set_name_f08(datatype, type_name, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_set_name_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_Datatype), intent(in) :: datatype
    character(len=*), intent(in) :: type_name
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    character(kind=c_char) :: type_name_c(len_trim(type_name)+1)
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(type_name, type_name_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_set_name_c(datatype%MPI_VAL, type_name_c)
    else
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Type_set_name_c(datatype_c, type_name_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_set_name_f08
