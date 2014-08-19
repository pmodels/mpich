!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_detach_f08ts(win, base, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Win
    use :: mpi_c_interface, only : c_Win
    use :: mpi_c_interface, only : MPIR_Win_detach_cdesc

    implicit none

    type(MPI_Win), intent(in) :: win
    type(*), dimension(..), asynchronous :: base
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_detach_cdesc(win%MPI_VAL, base)
    else
        win_c = win%MPI_VAL
        ierror_c = MPIR_Win_detach_cdesc(win_c, base)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_detach_f08ts
