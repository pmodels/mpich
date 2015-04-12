!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Pcontrol_f08(level)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Pcontrol_c

    implicit none

    integer, intent(in) :: level

    integer(c_int) :: level_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Pcontrol_c(level)
    else
        level_c = level
        ierror_c = MPIR_Pcontrol_c(level_c)
    end if


end subroutine MPI_Pcontrol_f08
