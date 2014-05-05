!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Buffer_detach_f08(buffer_addr, size, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface, only : MPIR_Buffer_detach_c

    implicit none

    type(c_ptr) :: buffer_addr
    integer, intent(out) :: size
    integer, optional, intent(out) :: ierror

    integer(c_int) :: size_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Buffer_detach_c(buffer_addr, size)
    else
        ierror_c = MPIR_Buffer_detach_c(buffer_addr, size_c)
        size = size_c
    end if

    if (present(ierror)) ierror = ierror_c
end subroutine MPI_Buffer_detach_f08
