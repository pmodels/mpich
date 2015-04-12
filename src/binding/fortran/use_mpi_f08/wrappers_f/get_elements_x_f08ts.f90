!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Get_elements_x_f08(status, datatype, count, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Status, MPI_Datatype
    use :: mpi_f08, only : MPI_COUNT_KIND
    use :: mpi_f08, only : assignment(=)
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Get_elements_x_c

    implicit none

    type(MPI_Status), intent(in), target :: status
    type(MPI_Datatype), intent(in) :: datatype
    integer(MPI_COUNT_KIND), intent(out) :: count
    integer, optional, intent(out) :: ierror

    type(c_Status), target :: status_c
    integer(c_Datatype) :: datatype_c
    integer(MPI_COUNT_KIND) :: count_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Get_elements_x_c(c_loc(status), datatype%MPI_VAL, count)
    else
        status_c = status
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Get_elements_x_c(c_loc(status_c), datatype_c, count_c)
        count = count_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Get_elements_x_f08
