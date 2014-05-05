!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Status_set_elements_f08(status, datatype, count, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Status, MPI_Datatype
    use :: mpi_f08, only : MPI_STATUS_IGNORE, MPIR_C_MPI_STATUS_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Status_set_elements_c

    implicit none

    type(MPI_Status), intent(inout), target :: status
    type(MPI_Datatype), intent(in) :: datatype
    integer, intent(in) :: count
    integer, optional, intent(out) :: ierror

    type(c_Status), target :: status_c
    integer(c_Datatype) :: datatype_c
    integer(c_int) :: count_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Status_set_elements_c(MPIR_C_MPI_STATUS_IGNORE, datatype%MPI_VAL, count)
        else
            ierror_c = MPIR_Status_set_elements_c(c_loc(status), datatype%MPI_VAL, count)
        end if
    else
        datatype_c = datatype%MPI_VAL
        count_c = count
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Status_set_elements_c(MPIR_C_MPI_STATUS_IGNORE, datatype_c, count_c)
        else
            ierror_c = MPIR_Status_set_elements_c(c_loc(status_c), datatype_c, count_c)
            status = status_c
        end if
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Status_set_elements_f08
