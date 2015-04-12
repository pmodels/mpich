!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Add_error_string_f08(errorcode, string, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface, only : c_Datatype, c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Add_error_string_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    integer, intent(in) :: errorcode
    character(len=*), intent(in) :: string
    integer, optional, intent(out) :: ierror

    integer(c_int) :: errorcode_c
    character(kind=c_char) :: string_c(len_trim(string)+1)
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(string, string_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Add_error_string_c(errorcode, string_c)
    else
        errorcode_c = errorcode
        ierror_c = MPIR_Add_error_string_c(errorcode_c, string_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Add_error_string_f08
