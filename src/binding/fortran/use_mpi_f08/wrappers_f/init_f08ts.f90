!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Init_f08(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_loc, C_NULL_PTR
    use :: mpi_c_interface, only : MPIR_Init_c

    implicit none

    integer, optional, intent(out) :: ierror
    integer(c_int) :: ierror_c

    ierror_c = MPIR_Init_c(C_NULL_PTR, C_NULL_PTR)

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Init_f08
