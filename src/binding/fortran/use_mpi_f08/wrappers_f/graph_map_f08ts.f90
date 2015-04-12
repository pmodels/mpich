!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Graph_map_f08(comm, nnodes, index, edges, newrank, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Graph_map_c

    implicit none

    type(MPI_Comm), intent(in)  :: comm
    integer, intent(in)  :: nnodes
    integer, intent(in)  :: index(nnodes)
    integer, intent(in)  :: edges(*)
    integer, intent(out)  :: newrank
    integer, optional, intent(out)  :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: nnodes_c
    integer(c_int) :: index_c(nnodes)
    integer(c_int) :: edges_c(index(nnodes))
    integer(c_int) :: newrank_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Graph_map_c(comm%MPI_VAL, nnodes, index, edges, newrank)
    else
        comm_c = comm%MPI_VAL
        nnodes_c = nnodes
        index_c = index
        edges_c = edges(1:index(nnodes))

        ierror_c = MPIR_Graph_map_c(comm_c, nnodes_c, index_c, edges_c, newrank_c)
        newrank = newrank_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Graph_map_f08
