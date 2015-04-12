!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Mrecv_f08ts(buf, count, datatype, message, status, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Datatype, MPI_Message, MPI_Status
    use :: mpi_f08, only : MPI_STATUS_IGNORE, MPIR_C_MPI_STATUS_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Datatype, c_Message
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Mrecv_cdesc

    implicit none

    type(*), dimension(..) :: buf
    integer, intent(in) :: count
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Message), intent(inout) :: message
    type(MPI_Status), target :: status
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_Datatype) :: datatype_c
    integer(c_Message) :: message_c
    type(c_Status), target :: status_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Mrecv_cdesc(buf, count, datatype%MPI_VAL, message%MPI_VAL, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Mrecv_cdesc(buf, count, datatype%MPI_VAL, message%MPI_VAL, c_loc(status))
        end if
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        message_c = message%MPI_VAL
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Mrecv_cdesc(buf, count_c, datatype_c, message_c, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Mrecv_cdesc(buf, count_c, datatype_c, message_c, c_loc(status_c))
            status = status_c
        end if
        message%MPI_VAL = message_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Mrecv_f08ts
