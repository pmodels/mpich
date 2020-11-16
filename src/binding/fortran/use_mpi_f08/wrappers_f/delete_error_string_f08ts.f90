!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

subroutine MPIX_Delete_error_string_f08(errorcode, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface, only : c_Datatype, c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Delete_error_string_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    integer, intent(in) :: errorcode
    integer, optional, intent(out) :: ierror

    integer(c_int) :: errorcode_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Delete_error_string_c(errorcode)
    else
        errorcode_c = errorcode
        ierror_c = MPIR_Delete_error_string_c(errorcode_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPIX_Delete_error_string_f08
