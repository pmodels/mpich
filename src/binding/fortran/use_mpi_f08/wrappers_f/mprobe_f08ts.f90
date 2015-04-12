!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Mprobe_f08(source, tag, comm, message, status, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Comm, MPI_Message, MPI_Status
    use :: mpi_f08, only : MPI_STATUS_IGNORE, MPIR_C_MPI_STATUS_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Comm, c_Message
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Mprobe_c

    implicit none

    integer, intent(in) :: source
    integer, intent(in) :: tag
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Message), intent(out) :: message
    type(MPI_Status), target :: status
    integer, optional, intent(out) :: ierror

    integer(c_int) :: source_c
    integer(c_int) :: tag_c
    integer(c_Comm) :: comm_c
    integer(c_Message) :: message_c
    type(c_Status), target :: status_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Mprobe_c(source, tag, comm%MPI_VAL, message%MPI_VAL, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Mprobe_c(source, tag, comm%MPI_VAL, message%MPI_VAL, c_loc(status))
        end if
    else
        source_c = source
        tag_c = tag
        comm_c = comm%MPI_VAL
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Mprobe_c(source_c, tag_c, comm_c, message_c, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Mprobe_c(source_c, tag_c, comm_c, message_c, c_loc(status_c))
            status = status_c
        end if
        message%MPI_VAL = message_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Mprobe_f08
