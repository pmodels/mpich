!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Testany_f08(count, array_of_requests, index, flag, status, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Request, MPI_Status
    use :: mpi_f08, only : MPI_STATUS_IGNORE, MPIR_C_MPI_STATUS_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Request
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Testany_c

    implicit none

    integer, intent(in) :: count
    type(MPI_Request), intent(inout) :: array_of_requests(count)
    integer, intent(out) :: index
    logical, intent(out) :: flag
    type(MPI_Status), target :: status
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_Request) :: array_of_requests_c(count)
    integer(c_int) :: index_c
    integer(c_int) :: flag_c
    type(c_Status), target :: status_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Testany_c(count, array_of_requests%MPI_VAL, index, flag_c, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Testany_c(count, array_of_requests%MPI_VAL, index, flag_c, c_loc(status))
        end if
    else
        count_c = count
        array_of_requests_c = array_of_requests%MPI_VAL
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Testany_c(count_c, array_of_requests_c, index_c, flag_c, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Testany_c(count_c, array_of_requests_c, index_c, flag_c, c_loc(status_c))
            status = status_c
        end if
        array_of_requests%MPI_VAL = array_of_requests_c
        index = index_c
    end if

    flag = (flag_c /= 0)
    if (index >= 0) index = index + 1
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Testany_f08
