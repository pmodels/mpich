!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Graph_neighbors_f08(comm, rank, maxneighbors, neighbors, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Graph_neighbors_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: rank
    integer, intent(in) :: maxneighbors
    integer, intent(out) :: neighbors(maxneighbors)
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: rank_c
    integer(c_int) :: maxneighbors_c
    integer(c_int) :: neighbors_c(maxneighbors)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Graph_neighbors_c(comm%MPI_VAL, rank, maxneighbors, neighbors)
    else
        comm_c = comm%MPI_VAL
        rank_c = rank
        maxneighbors_c = maxneighbors
        ierror_c = MPIR_Graph_neighbors_c(comm_c, rank_c, maxneighbors_c, neighbors_c)
        neighbors = neighbors_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Graph_neighbors_f08
