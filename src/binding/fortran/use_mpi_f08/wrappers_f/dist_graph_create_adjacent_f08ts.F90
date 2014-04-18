!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Dist_graph_create_adjacent_f08(comm_old, indegree, sources, sourceweights, &
    outdegree, destinations, destweights, info, reorder, comm_dist_graph, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr, c_loc, c_associated
    use :: mpi_f08, only : MPI_Comm, MPI_Info
    use :: mpi_f08, only : MPI_UNWEIGHTED, MPI_WEIGHTS_EMPTY, MPIR_C_MPI_UNWEIGHTED, MPIR_C_MPI_WEIGHTS_EMPTY
    use :: mpi_c_interface, only : c_Comm, c_Info
    use :: mpi_c_interface, only : MPIR_Dist_graph_create_adjacent_c

    implicit none

    type(MPI_Comm), intent(in) :: comm_old
    integer, intent(in) :: indegree
    integer, intent(in) :: sources(indegree)
    integer, intent(in), target :: sourceweights(indegree)
    integer, intent(in) :: outdegree
    integer, intent(in) :: destinations(outdegree)
    integer, intent(in), target :: destweights(outdegree)
    type(MPI_Info), intent(in) :: info
    logical, intent(in) :: reorder
    type(MPI_Comm), intent(out) :: comm_dist_graph
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_old_c
    integer(c_int) :: indegree_c
    integer(c_int) :: sources_c(indegree)
    integer(c_int), target :: sourceweights_c(indegree)
    integer(c_int) :: outdegree_c
    integer(c_int) :: destinations_c(outdegree)
    integer(c_int), target :: destweights_c(outdegree)
    integer(c_Info) :: info_c
    integer(c_int) :: reorder_c
    integer(c_Comm) :: comm_dist_graph_c
    integer(c_int) :: ierror_c

    type(c_ptr) :: sourceweights_cptr, destweights_cptr

    reorder_c = merge(1, 0, reorder)

    if (c_int == kind(0)) then
        if (c_associated(c_loc(sourceweights), c_loc(MPI_UNWEIGHTED))) then
            sourceweights_cptr = MPIR_C_MPI_UNWEIGHTED
        else if (c_associated(c_loc(sourceweights), c_loc(MPI_WEIGHTS_EMPTY))) then
            sourceweights_cptr = MPIR_C_MPI_WEIGHTS_EMPTY
        else
            sourceweights_cptr = c_loc(sourceweights)
        end if

        if (c_associated(c_loc(destweights), c_loc(MPI_UNWEIGHTED))) then
            destweights_cptr = MPIR_C_MPI_UNWEIGHTED
        else if (c_associated(c_loc(destweights), c_loc(MPI_WEIGHTS_EMPTY))) then
            destweights_cptr = MPIR_C_MPI_WEIGHTS_EMPTY
        else
            destweights_cptr = c_loc(destweights)
        end if

        ierror_c = MPIR_Dist_graph_create_adjacent_c(comm_old%MPI_VAL, indegree, sources, sourceweights_cptr, outdegree, &
                destinations, destweights_cptr, info%MPI_VAL, reorder_c, comm_dist_graph%MPI_VAL)
    else
        comm_old_c = comm_old%MPI_VAL
        indegree_c = indegree
        sources_c = sources
        outdegree_c = outdegree
        destinations_c = destinations
        info_c = info%MPI_VAL

        if (c_associated(c_loc(sourceweights), c_loc(MPI_UNWEIGHTED))) then
            sourceweights_cptr = MPIR_C_MPI_UNWEIGHTED
        else if (c_associated(c_loc(sourceweights), c_loc(MPI_WEIGHTS_EMPTY))) then
            sourceweights_cptr = MPIR_C_MPI_WEIGHTS_EMPTY
        else
            sourceweights_c = sourceweights
            sourceweights_cptr = c_loc(sourceweights_c)
        end if

        if (c_associated(c_loc(destweights), c_loc(MPI_UNWEIGHTED))) then
            destweights_cptr = MPIR_C_MPI_UNWEIGHTED
        else if (c_associated(c_loc(destweights), c_loc(MPI_WEIGHTS_EMPTY))) then
            destweights_cptr = MPIR_C_MPI_WEIGHTS_EMPTY
        else
            destweights_c = destweights
            destweights_cptr = c_loc(destweights_c)
        end if

        ierror_c = MPIR_Dist_graph_create_adjacent_c(comm_old_c, indegree_c, sources_c, sourceweights_cptr, outdegree_c, &
                destinations_c, destweights_cptr, info_c, reorder_c, comm_dist_graph_c)

        comm_dist_graph%MPI_VAL = comm_dist_graph_c
    endif

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Dist_graph_create_adjacent_f08
