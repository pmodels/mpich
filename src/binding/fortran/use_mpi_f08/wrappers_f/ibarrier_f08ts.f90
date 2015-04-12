!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Ibarrier_f08(comm, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Ibarrier_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Ibarrier_c(comm%MPI_VAL, request%MPI_VAL)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Ibarrier_c(comm_c, request_c)
        request%MPI_VAL = request_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Ibarrier_f08
