!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Dist_graph_create_f08(comm_old, n,sources, degrees, destinations, weights, &
    info, reorder, comm_dist_graph, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr, c_loc, c_associated
    use :: mpi_f08, only : MPI_Comm, MPI_Info
    use :: mpi_f08, only : MPI_UNWEIGHTED, MPI_WEIGHTS_EMPTY, MPIR_C_MPI_UNWEIGHTED, MPIR_C_MPI_WEIGHTS_EMPTY
    use :: mpi_c_interface, only : c_Comm, c_Info
    use :: mpi_c_interface, only : MPIR_Dist_graph_create_c

    implicit none

    type(MPI_Comm), intent(in) :: comm_old
    integer, intent(in) :: n
    integer, intent(in) :: sources(n)
    integer, intent(in) :: degrees(n)
    integer, intent(in) :: destinations(*)
    integer, intent(in), target :: weights(*)
    type(MPI_Info), intent(in) :: info
    logical, intent(in) :: reorder
    type(MPI_Comm), intent(out) :: comm_dist_graph
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_old_c
    integer(c_int) :: n_c
    integer(c_int) :: sources_c(n)
    integer(c_int) :: degrees_c(n)
    integer(c_int) :: destinations_c(sum(degrees))
    integer(c_int), target :: weights_c(sum(degrees))
    integer(c_Info) :: info_c
    integer(c_int) :: reorder_c
    integer(c_Comm) :: comm_dist_graph_c
    integer(c_int) :: ierror_c

    type(c_ptr) :: weights_cptr

    reorder_c = merge(1, 0, reorder)

    if (c_int == kind(0)) then
        if (c_associated(c_loc(weights), c_loc(MPI_UNWEIGHTED))) then
            weights_cptr = MPIR_C_MPI_UNWEIGHTED
        else if (c_associated(c_loc(weights), c_loc(MPI_WEIGHTS_EMPTY))) then
            weights_cptr = MPIR_C_MPI_WEIGHTS_EMPTY
        else
            weights_cptr = c_loc(weights)
        end if

        ierror_c = MPIR_Dist_graph_create_c(comm_old%MPI_VAL, n, sources, degrees, destinations, weights_cptr, &
            info%MPI_VAL, reorder_c, comm_dist_graph%MPI_VAL)
    else
        comm_old_c = comm_old%MPI_VAL
        n_c = n
        info_c = info%MPI_VAL
        sources_c = sources
        degrees_c = degrees

        if (c_associated(c_loc(weights), c_loc(MPI_UNWEIGHTED))) then
            weights_cptr = MPIR_C_MPI_UNWEIGHTED
        else if (c_associated(c_loc(weights), c_loc(MPI_WEIGHTS_EMPTY))) then
            weights_cptr = MPIR_C_MPI_WEIGHTS_EMPTY
        else
            weights_c = weights(1:sum(degrees))
            weights_cptr = c_loc(weights_c)
        end if

        destinations_c = destinations(1:sum(degrees))
        ierror_c = MPIR_Dist_graph_create_c(comm_old_c, n_c, sources_c, degrees_c, destinations_c, weights_cptr, &
            info_c, reorder_c, comm_dist_graph_c)

        comm_dist_graph%MPI_VAL = comm_dist_graph_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Dist_graph_create_f08
