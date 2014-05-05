!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Cart_shift_f08(comm, direction, disp, rank_source, rank_dest, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Cart_shift_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: direction
    integer, intent(in) :: disp
    integer, intent(out) :: rank_source
    integer, intent(out) :: rank_dest
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: direction_c
    integer(c_int) :: disp_c
    integer(c_int) :: rank_source_c
    integer(c_int) :: rank_dest_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Cart_shift_c(comm%MPI_VAL, direction, disp, rank_source, rank_dest)
    else
        comm_c = comm%MPI_VAL
        direction_c = direction
        disp_c = disp
        ierror_c = MPIR_Cart_shift_c(comm_c, direction_c, disp_c, rank_source_c, rank_dest_c)
        rank_source = rank_source_c
        rank_dest = rank_dest_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Cart_shift_f08
