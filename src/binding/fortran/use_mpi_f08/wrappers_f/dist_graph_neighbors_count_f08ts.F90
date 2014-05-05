!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Dist_graph_neighbors_count_f08(comm, indegree, outdegree, weighted, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Dist_graph_neighbors_count_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(out) :: indegree
    integer, intent(out) :: outdegree
    logical, intent(out) :: weighted
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: indegree_c
    integer(c_int) :: outdegree_c
    integer(c_int) :: weighted_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Dist_graph_neighbors_count_c(comm%MPI_VAL, indegree, outdegree, weighted_c)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Dist_graph_neighbors_count_c(comm_c, indegree_c, outdegree_c, weighted_c)
        indegree = indegree_c
        outdegree = outdegree_c
    end if

    weighted = (weighted_c /= 0)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Dist_graph_neighbors_count_f08
