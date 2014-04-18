!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Get_library_version_f08(version, resultlen, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_MAX_LIBRARY_VERSION_STRING
    use :: mpi_c_interface, only : MPIR_Get_library_version_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    character(len=MPI_MAX_LIBRARY_VERSION_STRING), intent(out) :: version
    integer, intent(out) :: resultlen
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: version_c(MPI_MAX_LIBRARY_VERSION_STRING+1)
    integer(c_int) :: resultlen_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Get_library_version_c(version_c, resultlen)
    else
        ierror_c = MPIR_Get_library_version_c(version_c, resultlen_c)
        resultlen = resultlen_c
    end if

    call MPIR_Fortran_string_c2f(version_c, version)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Get_library_version_f08
