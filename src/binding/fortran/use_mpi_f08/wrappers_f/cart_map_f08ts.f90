!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Cart_map_f08(comm, ndims, dims, periods, newrank, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Cart_map_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: ndims
    integer, intent(in) :: dims(ndims)
    logical, intent(in) :: periods(ndims)
    integer, intent(out) :: newrank
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: ndims_c
    integer(c_int) :: dims_c(ndims)
    integer(c_int) :: periods_c(ndims)
    integer(c_int) :: newrank_c
    integer(c_int) :: ierror_c

    periods_c = merge(1, 0, periods)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Cart_map_c(comm%MPI_VAL, ndims, dims, periods_c, newrank)
    else
        comm_c = comm%MPI_VAL
        ndims_c = ndims
        dims_c = dims
        ierror_c = MPIR_Cart_map_c(comm_c, ndims_c, dims_c, periods_c, newrank_c)
        newrank = newrank_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Cart_map_f08
