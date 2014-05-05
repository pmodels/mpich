!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_get_group_f08(win, group, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Win, MPI_Group
    use :: mpi_c_interface, only : c_Win, c_Group
    use :: mpi_c_interface, only : MPIR_Win_get_group_c

    implicit none

    type(MPI_Win), intent(in) :: win
    type(MPI_Group), intent(out) :: group
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    integer(c_Group) :: group_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_get_group_c(win%MPI_VAL, group%MPI_VAL)
    else
        win_c = win%MPI_VAL
        ierror_c = MPIR_Win_get_group_c(win_c, group_c)
        group%MPI_VAL = group_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_get_group_f08
