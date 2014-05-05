!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Startall_f08(count, array_of_requests, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Request
    use :: mpi_c_interface, only : c_Request
    use :: mpi_c_interface, only : MPIR_Startall_c

    implicit none

    integer, intent(in) :: count
    type(MPI_Request), intent(inout) :: array_of_requests(count)
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_Request) :: array_of_requests_c(count)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Startall_c(count, array_of_requests%MPI_VAL)
    else
        count_c = count
        array_of_requests_c = array_of_requests%MPI_VAL
        ierror_c = MPIR_Startall_c(count_c, array_of_requests_c)
        array_of_requests%MPI_VAL = array_of_requests_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Startall_f08
