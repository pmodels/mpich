!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_set_attr_f08(win, win_keyval, attribute_val, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Win
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Win
    use :: mpi_c_interface, only : MPIR_ATTR_AINT
    use :: mpi_c_interface, only : MPIR_Win_set_attr_c

    implicit none

    type(MPI_Win), intent(in) :: win
    integer, intent(in) :: win_keyval
    integer(MPI_ADDRESS_KIND), intent(in) :: attribute_val
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    integer(c_int) :: win_keyval_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_set_attr_c(win%MPI_VAL, win_keyval, attribute_val, MPIR_ATTR_AINT)
    else
        win_c = win%MPI_VAL
        win_keyval_c = win_keyval
        ierror_c = MPIR_Win_set_attr_c(win_c, win_keyval_c, attribute_val, MPIR_ATTR_AINT)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_set_attr_f08
