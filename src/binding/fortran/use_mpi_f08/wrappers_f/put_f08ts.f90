!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Put_f08ts(origin_addr, origin_count, origin_datatype, target_rank, &
    target_disp, target_count, target_datatype, win, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Win
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype, c_Win
    use :: mpi_c_interface, only : MPIR_Put_cdesc

    implicit none

    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    integer, intent(in) :: origin_count
    integer, intent(in) :: target_rank
    integer, intent(in) :: target_count
    type(MPI_Datatype), intent(in) :: origin_datatype
    integer(MPI_ADDRESS_KIND), intent(in) :: target_disp
    type(MPI_Datatype), intent(in) :: target_datatype
    type(MPI_Win), intent(in) :: win
    integer, optional, intent(out) :: ierror

    integer(c_int) :: origin_count_c
    integer(c_int) :: target_rank_c
    integer(c_int) :: target_count_c
    integer(c_Datatype) :: origin_datatype_c
    integer(MPI_ADDRESS_KIND) :: target_disp_c
    integer(c_Datatype) :: target_datatype_c
    integer(c_Win) :: win_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Put_cdesc(origin_addr, origin_count, origin_datatype%MPI_VAL, target_rank, target_disp, &
            target_count, target_datatype%MPI_VAL, win%MPI_VAL)
    else
        origin_count_c = origin_count
        origin_datatype_c = origin_datatype%MPI_VAL
        target_rank_c = target_rank
        target_disp_c = target_disp
        target_count_c = target_count
        target_datatype_c = target_datatype%MPI_VAL
        win_c = win%MPI_VAL
        ierror_c = MPIR_Put_cdesc(origin_addr, origin_count_c, origin_datatype_c, target_rank_c, target_disp_c, &
            target_count_c, target_datatype_c, win_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Put_f08ts
