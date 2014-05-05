!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Grequest_complete_f08(request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Request
    use :: mpi_c_interface, only : c_Request
    use :: mpi_c_interface, only : MPIR_Grequest_complete_c

    implicit none

    type(MPI_Request), intent(in) :: request
    integer, optional, intent(out) :: ierror

    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Grequest_complete_c(request%MPI_VAL)
    else
        request_c = request%MPI_VAL
        ierror_c = MPIR_Grequest_complete_c(request_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Grequest_complete_f08
