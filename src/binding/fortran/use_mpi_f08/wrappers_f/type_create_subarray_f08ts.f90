!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_create_subarray_f08(ndims, array_of_sizes, array_of_subsizes, &
    array_of_starts, order, oldtype, newtype, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_create_subarray_c

    implicit none

    integer, intent(in) :: ndims
    integer, intent(in) :: order
    integer, intent(in) :: array_of_sizes(ndims)
    integer, intent(in) :: array_of_subsizes(ndims)
    integer, intent(in) :: array_of_starts(ndims)
    type(MPI_Datatype), intent(in) :: oldtype
    type(MPI_Datatype), intent(out) :: newtype
    integer, optional, intent(out) :: ierror

    integer(c_int) :: ndims_c
    integer(c_int) :: order_c
    integer(c_int) :: array_of_sizes_c(ndims)
    integer(c_int) :: array_of_subsizes_c(ndims)
    integer(c_int) :: array_of_starts_c(ndims)
    integer(c_Datatype) :: oldtype_c
    integer(c_Datatype) :: newtype_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_create_subarray_c(ndims, array_of_sizes, array_of_subsizes, array_of_starts, &
            order, oldtype%MPI_VAL, newtype%MPI_VAL)
    else
        ndims_c = ndims
        array_of_sizes_c = array_of_sizes
        array_of_subsizes_c = array_of_subsizes
        array_of_starts_c = array_of_starts
        order_c = order
        oldtype_c = oldtype%MPI_VAL
        ierror_c = MPIR_Type_create_subarray_c(ndims_c, array_of_sizes_c, array_of_subsizes_c, array_of_starts_c, &
            order_c, oldtype_c, newtype_c)
        newtype%MPI_VAL = newtype_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_create_subarray_f08
