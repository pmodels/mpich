!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Cart_create_f08(comm_old, ndims, dims, periods, reorder, comm_cart, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Cart_create_c

    implicit none

    type(MPI_Comm), intent(in) :: comm_old
    integer, intent(in) :: ndims
    integer, intent(in) :: dims(ndims)
    logical, intent(in) :: periods(ndims)
    logical, intent(in) :: reorder
    type(MPI_Comm), intent(out) :: comm_cart
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_old_c
    integer(c_int) :: ndims_c
    integer(c_int) :: dims_c(ndims)
    integer(c_int) :: periods_c(ndims)
    integer(c_int) :: reorder_c
    integer(c_Comm) :: comm_cart_c
    integer(c_int) :: ierror_c

    periods_c = merge(1, 0, periods)
    reorder_c = merge(1, 0, reorder)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Cart_create_c(comm_old%MPI_VAL, ndims, dims, periods_c, reorder_c, comm_cart%MPI_VAL)
    else
        comm_old_c = comm_old%MPI_VAL
        ndims_c = ndims
        dims_c = dims
        ierror_c = MPIR_Cart_create_c(comm_old_c, ndims_c, dims_c, periods_c, reorder_c, comm_cart_c)
        comm_cart%MPI_VAL = comm_cart_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Cart_create_f08
