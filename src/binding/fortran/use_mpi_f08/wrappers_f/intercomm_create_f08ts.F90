!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Intercomm_create_f08(local_comm, local_leader, peer_comm, remote_leader, &
    tag, newintercomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Intercomm_create_c

    implicit none

    type(MPI_Comm), intent(in) :: local_comm
    type(MPI_Comm), intent(in) :: peer_comm
    integer, intent(in) :: local_leader
    integer, intent(in) :: remote_leader
    integer, intent(in) :: tag
    type(MPI_Comm), intent(out) :: newintercomm
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: local_comm_c
    integer(c_Comm) :: peer_comm_c
    integer(c_int) :: local_leader_c
    integer(c_int) :: remote_leader_c
    integer(c_int) :: tag_c
    integer(c_Comm) :: newintercomm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Intercomm_create_c(local_comm%MPI_VAL, local_leader, peer_comm%MPI_VAL, remote_leader, &
            tag, newintercomm%MPI_VAL)
    else
        local_comm_c = local_comm%MPI_VAL
        local_leader_c = local_leader
        peer_comm_c = peer_comm%MPI_VAL
        remote_leader_c = remote_leader
        tag_c = tag
        ierror_c = MPIR_Intercomm_create_c(local_comm_c, local_leader_c, peer_comm_c, remote_leader_c, &
            tag_c, newintercomm_c)
        newintercomm%MPI_VAL = newintercomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Intercomm_create_f08
