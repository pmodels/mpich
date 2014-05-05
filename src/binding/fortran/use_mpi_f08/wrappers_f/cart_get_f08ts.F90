!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Cart_get_f08(comm, maxdims, dims, periods, coords, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Cart_get_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: maxdims
    integer, intent(out) :: dims(maxdims)
    integer, intent(out) :: coords(maxdims)
    logical, intent(out) :: periods(maxdims)
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: maxdims_c
    integer(c_int) :: dims_c(maxdims)
    integer(c_int) :: coords_c(maxdims)
    integer(c_int) :: periods_c(maxdims)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Cart_get_c(comm%MPI_VAL, maxdims, dims, periods_c, coords)
    else
        comm_c = comm%MPI_VAL
        maxdims_c = maxdims
        ierror_c = MPIR_Cart_get_c(comm_c, maxdims_c, dims_c, periods_c, coords_c)
        dims = dims_c
        coords = coords_c
    end if

    periods = (periods_c /= 0)

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Cart_get_f08
