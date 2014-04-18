!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Get_version_f08(version, subversion, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Get_version_c

    implicit none

    integer, intent(out) :: version
    integer, intent(out) :: subversion
    integer, optional, intent(out) :: ierror

    integer(c_int) :: version_c
    integer(c_int) :: subversion_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Get_version_c(version, subversion)
    else
        ierror_c = MPIR_Get_version_c(version_c, subversion_c)
        version = version_c
        subversion = subversion_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Get_version_f08
