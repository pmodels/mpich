!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_get_extent_f08(datatype, lb, extent, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_get_extent_c

    implicit none

    type(MPI_Datatype), intent(in) :: datatype
    integer(MPI_ADDRESS_KIND), intent(out) :: lb
    integer(MPI_ADDRESS_KIND), intent(out) :: extent
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    integer(MPI_ADDRESS_KIND) :: lb_c
    integer(MPI_ADDRESS_KIND) :: extent_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_get_extent_c(datatype%MPI_VAL, lb, extent)
    else
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Type_get_extent_c(datatype_c, lb_c, extent_c)
        lb = lb_c
        extent = extent_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_get_extent_f08
