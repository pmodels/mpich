!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_post_f08(group, assert, win, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Group, MPI_Win
    use :: mpi_c_interface, only : c_Win, c_Group
    use :: mpi_c_interface, only : MPIR_Win_post_c

    implicit none

    type(MPI_Group), intent(in) :: group
    integer, intent(in) :: assert
    type(MPI_Win), intent(in) :: win
    integer, optional, intent(out) :: ierror

    integer(c_Group) :: group_c
    integer(c_int) :: assert_c
    integer(c_Win) :: win_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_post_c(group%MPI_VAL, assert, win%MPI_VAL)
    else
        group_c = group%MPI_VAL
        assert_c = assert
        win_c = win%MPI_VAL
        ierror_c = MPIR_Win_post_c(group_c, assert_c, win_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_post_f08
