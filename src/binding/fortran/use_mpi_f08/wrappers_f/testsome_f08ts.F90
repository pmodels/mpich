!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Testsome_f08(incount, array_of_requests, outcount, &
    array_of_indices, array_of_statuses, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Request, MPI_Status
    use :: mpi_f08, only : MPI_STATUSES_IGNORE, MPIR_C_MPI_STATUSES_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Request
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Testsome_c

    implicit none

    integer, intent(in) :: incount
    type(MPI_Request), intent(inout) :: array_of_requests(incount)
    integer, intent(out) :: outcount
    integer, intent(out) :: array_of_indices(*)
    type(MPI_Status), target :: array_of_statuses(*)
    integer, optional, intent(out) :: ierror

    integer(c_int) :: incount_c
    integer(c_Request) :: array_of_requests_c(incount)
    integer(c_int) :: outcount_c
    integer(c_int) :: array_of_indices_c(incount)
    type(c_Status), target :: array_of_statuses_c(incount)
    integer(c_int) :: ierror_c
    integer :: i

    if (c_int == kind(0)) then
        if (c_associated(c_loc(array_of_statuses), c_loc(MPI_STATUSES_IGNORE))) then
            ierror_c = MPIR_Testsome_c(incount, array_of_requests%MPI_VAL, outcount, array_of_indices, MPIR_C_MPI_STATUSES_IGNORE)
        else
            ierror_c = MPIR_Testsome_c(incount, array_of_requests%MPI_VAL, outcount, array_of_indices, c_loc(array_of_statuses))
        end if
    else
        incount_c = incount
        array_of_requests_c = array_of_requests%MPI_VAL
        if (c_associated(c_loc(array_of_statuses), c_loc(MPI_STATUSES_IGNORE))) then
            ierror_c = MPIR_Testsome_c(incount_c, array_of_requests_c, outcount_c, array_of_indices_c, MPIR_C_MPI_STATUSES_IGNORE)
        else
            ierror_c = MPIR_Testsome_c(incount_c, array_of_requests_c, outcount_c, array_of_indices_c, c_loc(array_of_statuses_c))
            array_of_statuses(1:outcount_c) = array_of_statuses_c(1:outcount_c)
            array_of_indices(1:outcount_c) = array_of_indices_c(1:outcount_c)
        end if
        array_of_requests%MPI_VAL = array_of_requests_c
        outcount = outcount_c
    end if

    do i = 1, outcount
        if (array_of_indices(i) >= 0) then
            array_of_indices(i) = array_of_indices(i) + 1
        end if
    end do

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Testsome_f08
