!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_flush_f08(rank, win, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Win
    use :: mpi_c_interface_types, only : c_Win
    use :: mpi_c_interface, only : MPIR_Win_flush_c

    implicit none

    integer, intent(in) :: rank
    type(MPI_Win), intent(in) :: win
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    integer(c_int) :: rank_c, ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_flush_c(rank, win%MPI_VAL)
    else
        rank_c = rank
        win_c = win%MPI_VAL
        ierror_c = MPIR_Win_flush_c(rank_c, win_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_flush_f08
