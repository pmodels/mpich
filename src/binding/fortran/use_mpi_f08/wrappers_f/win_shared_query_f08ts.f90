!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_shared_query_f08(win, rank, size, disp_unit, baseptr, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Win
    use :: mpi_c_interface, only : MPIR_Win_shared_query_c

    implicit none

    type(MPI_Win), intent(in) :: win
    integer, intent(in) :: rank
    integer(kind=MPI_ADDRESS_KIND), intent(out) :: size
    integer, intent(out) :: disp_unit
    type(c_ptr), intent(out) :: baseptr
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    integer(c_int) :: rank_c, disp_unit_c, ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_shared_query_c(win%MPI_VAL, rank, size, disp_unit, baseptr)
    else
        win_c = win%MPI_VAL
        rank_c = rank
        ierror_c = MPIR_Win_shared_query_c(win_c, rank_c, size, disp_unit_c, baseptr)
        disp_unit = disp_unit_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_shared_query_f08
