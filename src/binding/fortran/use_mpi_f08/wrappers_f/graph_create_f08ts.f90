!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Graph_create_f08(comm_old, nnodes, index, edges, reorder, comm_graph, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Graph_create_c

    implicit none

    type(MPI_Comm), intent(in) :: comm_old
    integer, intent(in) :: nnodes
    integer, intent(in) :: index(nnodes)
    integer, intent(in) :: edges(*)
    logical, intent(in) :: reorder
    type(MPI_Comm), intent(out) :: comm_graph
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_old_c
    integer(c_int) :: nnodes_c
    integer(c_int) :: index_c(nnodes)
    integer(c_int), allocatable, dimension(:) :: edges_c
    integer(c_int) :: reorder_c
    integer(c_Comm) :: comm_graph_c
    integer(c_int) :: ierror_c
    integer(c_int) :: length ! To get length of assumed-size arrays

    reorder_c = merge(1, 0, reorder)
    if (c_int == kind(0)) then
        ierror_c = MPIR_Graph_create_c(comm_old%MPI_VAL, nnodes, index, edges, reorder_c, comm_graph%MPI_VAL)
    else
        length = index(nnodes)
        comm_old_c = comm_old%MPI_VAL
        nnodes_c = nnodes
        index_c = index
        edges_c(1:length) = edges(1:length)
        ierror_c = MPIR_Graph_create_c(comm_old_c, nnodes_c, index_c, edges_c, reorder_c, comm_graph_c)
        comm_graph%MPI_VAL = comm_graph_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Graph_create_f08
