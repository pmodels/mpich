!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Cartdim_get_f08(comm, ndims, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Cartdim_get_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(out) :: ndims
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: ndims_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Cartdim_get_c(comm%MPI_VAL, ndims)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Cartdim_get_c(comm_c, ndims_c)
        ndims = ndims_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Cartdim_get_f08
