!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Pack_external_size_f08(datarep, incount, datatype, size, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Pack_external_size_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_Datatype), intent(in) :: datatype
    integer, intent(in) :: incount
    character(len=*), intent(in) :: datarep
    integer(MPI_ADDRESS_KIND), intent(out) :: size
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    integer(c_int) :: incount_c
    character(kind=c_char) :: datarep_c(len_trim(datarep)+1)
    integer(MPI_ADDRESS_KIND) :: size_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(datarep, datarep_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Pack_external_size_c(datarep_c, incount, datatype%MPI_VAL, size)
    else
        incount_c = incount
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Pack_external_size_c(datarep_c, incount_c, datatype_c, size_c)
        size = size_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Pack_external_size_f08
