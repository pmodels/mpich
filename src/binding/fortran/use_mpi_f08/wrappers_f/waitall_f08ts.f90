!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Waitall_f08(count, array_of_requests, array_of_statuses, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Request, MPI_Status
    use :: mpi_f08, only : MPI_STATUSES_IGNORE, MPIR_C_MPI_STATUSES_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Request
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Waitall_c

    implicit none

    integer, intent(in) :: count
    type(MPI_Request), intent(inout) :: array_of_requests(count)
    type(MPI_Status), target :: array_of_statuses(*)
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_Request) :: array_of_requests_c(count)
    type(c_Status), target :: array_of_statuses_c(count)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        if (c_associated(c_loc(array_of_statuses), c_loc(MPI_STATUSES_IGNORE))) then
            ierror_c = MPIR_Waitall_c(count, array_of_requests%MPI_VAL, MPIR_C_MPI_STATUSES_IGNORE)
        else
            ierror_c = MPIR_Waitall_c(count, array_of_requests%MPI_VAL, c_loc(array_of_statuses))
        end if
    else
        count_c = count
        array_of_requests_c = array_of_requests%MPI_VAL
        if (c_associated(c_loc(array_of_statuses), c_loc(MPI_STATUSES_IGNORE))) then
            ierror_c = MPIR_Waitall_c(count_c, array_of_requests_c, MPIR_C_MPI_STATUSES_IGNORE)
        else
            ierror_c = MPIR_Waitall_c(count_c, array_of_requests_c, c_loc(array_of_statuses_c))
            array_of_statuses(1:count_c) = array_of_statuses_c(1:count_c)
        end if
        array_of_requests%MPI_VAL = array_of_requests_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Waitall_f08
