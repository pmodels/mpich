!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_split_f08(comm, color, key, newcomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_split_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: color
    integer, intent(in) :: key
    type(MPI_Comm), intent(out) :: newcomm
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: color_c
    integer(c_int) :: key_c
    integer(c_Comm) :: newcomm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_split_c(comm%MPI_VAL, color, key, newcomm%MPI_VAL)
    else
        comm_c = comm%MPI_VAL
        color_c = color
        key_c = key
        ierror_c = MPIR_Comm_split_c(comm_c, color_c, key_c, newcomm_c)
        newcomm%MPI_VAL = newcomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_split_f08
