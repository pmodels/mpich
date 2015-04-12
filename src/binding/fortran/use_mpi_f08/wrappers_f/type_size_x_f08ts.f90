!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_size_x_f08(datatype, size, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_COUNT_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_size_x_c

    implicit none

    type(MPI_Datatype), intent(in) :: datatype
    integer(MPI_COUNT_KIND), intent(out) :: size
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    integer(MPI_COUNT_KIND) :: size_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_size_x_c(datatype%MPI_VAL, size)
    else
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Type_size_x_c(datatype_c, size_c)
        size = size_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_size_x_f08
