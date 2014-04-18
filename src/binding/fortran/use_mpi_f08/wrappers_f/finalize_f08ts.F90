!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Finalize_f08(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Finalize_c

    implicit none

    integer, optional, intent(out) :: ierror

    integer(c_int) :: ierror_c

    ierror_c = MPIR_Finalize_c()

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Finalize_f08
