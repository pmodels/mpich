!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_remote_size_f08(comm, size, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_remote_size_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(out) :: size
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: size_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_remote_size_c(comm%MPI_VAL, size)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Comm_remote_size_c(comm_c, size_c)
        size = size_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_remote_size_f08
