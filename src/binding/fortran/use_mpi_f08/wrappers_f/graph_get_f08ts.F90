!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Graph_get_f08(comm, maxindex, maxedges, index, edges, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Graph_get_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: maxindex
    integer, intent(in) :: maxedges
    integer, intent(out) :: index(maxindex)
    integer, intent(out) :: edges(maxedges)
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: maxindex_c
    integer(c_int) :: maxedges_c
    integer(c_int) :: index_c(maxindex)
    integer(c_int) :: edges_c(maxedges)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Graph_get_c(comm%MPI_VAL, maxindex, maxedges, index, edges)
    else
        comm_c = comm%MPI_VAL
        maxindex_c = maxindex
        maxedges_c = maxedges
        ierror_c = MPIR_Graph_get_c(comm_c, maxindex_c, maxedges_c, index_c, edges_c)
        index = index_c
        edges = edges_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Graph_get_f08
