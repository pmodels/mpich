!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Graphdims_get_f08(comm, nnodes, nedges, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Graphdims_get_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(out) :: nnodes
    integer, intent(out) :: nedges
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: nnodes_c
    integer(c_int) :: nedges_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Graphdims_get_c(comm%MPI_VAL, nnodes, nedges)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Graphdims_get_c(comm_c, nnodes_c, nedges_c)
        nnodes = nnodes_c
        nedges = nedges_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Graphdims_get_f08
