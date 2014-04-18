!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Initialized_f08(flag, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Initialized_c

    implicit none

    logical, intent(out) :: flag
    integer, optional, intent(out) :: ierror

    integer(c_int) :: flag_c
    integer(c_int) :: ierror_c

    ierror_c = MPIR_Initialized_c(flag_c)

    flag = (flag_c /= 0)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Initialized_f08
