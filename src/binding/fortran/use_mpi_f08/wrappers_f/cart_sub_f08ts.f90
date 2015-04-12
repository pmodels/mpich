!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Cart_sub_f08(comm, remain_dims, newcomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm, MPI_SUCCESS
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Cart_sub_c, MPIR_Cartdim_get_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    logical, intent(in) :: remain_dims(*)
    type(MPI_Comm), intent(out) :: newcomm
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int), allocatable :: remain_dims_c(:)
    integer(c_Comm) :: newcomm_c
    integer(c_int) :: ierror_c
    integer(c_int) :: err, ndims! To get length of assumed-size arrays

    comm_c = comm%MPI_VAL
    ierror_c = MPIR_Cartdim_get_c(comm_c, ndims)

    if (ierror_c /= MPI_SUCCESS) then
        remain_dims_c = merge(1, 0, remain_dims(1:ndims))
        ierror_c = MPIR_Cart_sub_c(comm_c, remain_dims_c, newcomm_c)
        newcomm%MPI_VAL = newcomm_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Cart_sub_f08
