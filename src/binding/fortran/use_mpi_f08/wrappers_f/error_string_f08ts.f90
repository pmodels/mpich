!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Error_string_f08(errorcode, string, resultlen, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_MAX_ERROR_STRING
    use :: mpi_f08, only : MPI_MAX_ERROR_STRING
    use :: mpi_c_interface, only : MPIR_Error_string_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    integer, intent(in) :: errorcode
    character(len=MPI_MAX_ERROR_STRING), intent(out) :: string
    integer, intent(out) :: resultlen
    integer, optional, intent(out) :: ierror

    integer(c_int) :: errorcode_c
    character(kind=c_char) :: string_c(MPI_MAX_ERROR_STRING+1)
    integer(c_int) :: resultlen_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Error_string_c(errorcode, string_c, resultlen)
    else
        errorcode_c = errorcode
        ierror_c = MPIR_Error_string_c(errorcode_c, string_c, resultlen_c)
        resultlen = resultlen_c
    end if

    call MPIR_Fortran_string_c2f(string_c, string)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Error_string_f08
