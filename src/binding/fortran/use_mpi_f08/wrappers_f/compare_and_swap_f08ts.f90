!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Compare_and_swap_f08ts(origin_addr, compare_addr, result_addr, datatype, &
    target_rank, target_disp, win, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype, c_Win
    use :: mpi_c_interface, only : MPIR_Compare_and_swap_cdesc

    implicit none

    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    type(*), dimension(..), intent(in), asynchronous :: compare_addr
    type(*), dimension(..), asynchronous :: result_addr
    type(MPI_Datatype), intent(in) :: datatype
    integer, intent(in) :: target_rank
    integer(kind=MPI_ADDRESS_KIND), intent(in) :: target_disp
    type(MPI_Win), intent(in) :: win
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    integer(c_int) :: target_rank_c
    integer(c_Win) :: win_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Compare_and_swap_cdesc(origin_addr, compare_addr, result_addr, datatype%MPI_VAL, &
            target_rank, target_disp, win%MPI_VAL)
    else
        datatype_c = datatype%MPI_VAL
        target_rank_c = target_rank
        win_c = win%MPI_VAL
        ierror_c = MPIR_Compare_and_swap_cdesc(origin_addr, compare_addr, result_addr, datatype_c, &
            target_rank_c, target_disp, win_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Compare_and_swap_f08ts
