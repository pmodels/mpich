!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Get_address_f08ts(location, address, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : MPIR_Get_address_cdesc

    implicit none

    type(*), dimension(..), asynchronous :: location
    integer(MPI_ADDRESS_KIND), intent(out) :: address
    integer, optional, intent(out) :: ierror

    integer(c_int) :: ierror_c

    ierror_c = MPIR_Get_address_cdesc(location, address)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Get_address_f08ts
