!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_idup_f08(comm, newcomm, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Comm_idup_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    type(MPI_Comm), intent(out) :: newcomm
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_Comm) :: newcomm_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_idup_c(comm%MPI_VAL, newcomm%MPI_VAL, request%MPI_VAL)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Comm_idup_c(comm_c, newcomm_c, request_c)
        newcomm%MPI_VAL = newcomm_c
        request%MPI_VAL = request_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_idup_f08
