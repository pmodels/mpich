!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_allocate_shared_f08(size, disp_unit, info, comm, baseptr, win, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Info, MPI_Comm, MPI_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Info, c_Comm, c_Win
    use :: mpi_c_interface, only : MPIR_Win_allocate_shared_c

    implicit none

    integer(kind=MPI_ADDRESS_KIND), intent(in) :: size
    integer, intent(in) :: disp_unit
    type(MPI_Info), intent(in) :: info
    type(MPI_Comm), intent(in) :: comm
    type(c_ptr), intent(out) :: baseptr
    type(MPI_Win), intent(out) :: win
    integer, optional, intent(out) :: ierror

    integer(c_int) :: disp_unit_c
    integer(c_Info) :: info_c
    integer(c_Comm) :: comm_c
    type(c_ptr) :: baseptr_c
    integer(c_Win) :: win_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_allocate_shared_c(size, disp_unit, info%MPI_VAL, comm%MPI_VAL, baseptr, win%MPI_VAL)
    else
        disp_unit_c = disp_unit
        info_c = info%MPI_VAL
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Win_allocate_shared_c(size, disp_unit_c, info_c, comm_c, baseptr_c, win_c)
        baseptr = baseptr_c
        win%MPI_VAL = win_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_allocate_shared_f08
