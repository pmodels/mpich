!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Cart_coords_f08(comm, rank, maxdims, coords, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Cart_coords_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: rank
    integer, intent(in) :: maxdims
    integer, intent(out) :: coords(maxdims)
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: rank_c
    integer(c_int) :: maxdims_c
    integer(c_int) :: coords_c(maxdims)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Cart_coords_c(comm%MPI_VAL, rank, maxdims, coords)
    else
        comm_c = comm%MPI_VAL
        rank_c = rank
        maxdims_c = maxdims
        ierror_c = MPIR_Cart_coords_c(comm_c, rank_c, maxdims_c, coords_c)
        coords = coords_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Cart_coords_f08
