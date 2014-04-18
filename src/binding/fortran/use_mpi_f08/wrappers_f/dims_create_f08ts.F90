!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Dims_create_f08(nnodes, ndims, dims, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Dims_create_c

    implicit none

    integer, intent(in) :: nnodes
    integer, intent(in) :: ndims
    integer, intent(inout) :: dims(ndims)
    integer, optional, intent(out) :: ierror

    integer(c_int) :: nnodes_c
    integer(c_int) :: ndims_c
    integer(c_int) :: dims_c(ndims)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Dims_create_c(nnodes, ndims, dims)
    else
        nnodes_c = nnodes
        ndims_c = ndims
        dims_c = dims
        ierror_c = MPIR_Dims_create_c(nnodes_c, ndims_c, dims_c)
        dims = dims_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Dims_create_f08
