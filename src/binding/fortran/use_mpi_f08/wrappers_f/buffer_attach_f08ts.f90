!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Buffer_attach_f08ts(buffer, size, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Buffer_attach_cdesc

    implicit none

    type(*), dimension(..) :: buffer
    integer, intent(in) :: size
    integer, optional, intent(out) :: ierror

    integer(c_int) :: size_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Buffer_attach_cdesc(buffer, size)
    else
        size_c = size
        ierror_c = MPIR_Buffer_attach_cdesc(buffer, size_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Buffer_attach_f08ts
