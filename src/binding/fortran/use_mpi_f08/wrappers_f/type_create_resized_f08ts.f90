!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_create_resized_f08(oldtype, lb, extent, newtype, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_create_resized_c

    implicit none

    integer(MPI_ADDRESS_KIND), intent(in) :: lb
    integer(MPI_ADDRESS_KIND), intent(in) :: extent
    type(MPI_Datatype), intent(in) :: oldtype
    type(MPI_Datatype), intent(out) :: newtype
    integer, optional, intent(out) :: ierror

    integer(MPI_ADDRESS_KIND) :: lb_c
    integer(MPI_ADDRESS_KIND) :: extent_c
    integer(c_Datatype) :: oldtype_c
    integer(c_Datatype) :: newtype_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_create_resized_c(oldtype%MPI_VAL, lb, extent, newtype%MPI_VAL)
    else
        oldtype_c = oldtype%MPI_VAL
        lb_c = lb
        extent_c = extent
        ierror_c = MPIR_Type_create_resized_c(oldtype_c, lb_c, extent_c, newtype_c)
        newtype%MPI_VAL = newtype_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_create_resized_f08
