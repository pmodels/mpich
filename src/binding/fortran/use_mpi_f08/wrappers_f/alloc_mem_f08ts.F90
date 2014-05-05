!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Alloc_mem_f08(size, info, baseptr, ierror)
    use, intrinsic :: iso_c_binding, only : c_ptr
    use, intrinsic :: iso_c_binding, only : c_ptr, c_int
    use :: mpi_f08, only : MPI_Info
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_Alloc_mem_c

    implicit none

    integer(MPI_ADDRESS_KIND), intent(in) :: size
    type(MPI_Info), intent(in) :: info
    type(c_ptr), intent(out) :: baseptr
    integer, optional, intent(out) :: ierror

    integer(MPI_ADDRESS_KIND) :: size_c
    integer(c_Info) :: info_c
    type(c_ptr) :: baseptr_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Alloc_mem_c(size, info%MPI_VAL, baseptr)
    else
        size_c = size
        info_c = info%MPI_VAL
        ierror_c = MPIR_Alloc_mem_c(size_c, info_c, baseptr_c)
        baseptr = baseptr_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Alloc_mem_f08
