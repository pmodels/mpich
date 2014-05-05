!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Imrecv_f08ts(buf, count, datatype, message, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Message, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Message, c_Request
    use :: mpi_c_interface, only : MPIR_Imrecv_cdesc

    implicit none

    type(*), dimension(..), asynchronous :: buf
    integer, intent(in) :: count
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Message), intent(inout) :: message
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_Datatype) :: datatype_c
    integer(c_Message) :: message_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Imrecv_cdesc(buf, count, datatype%MPI_VAL, message%MPI_VAL, request%MPI_VAL)
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        message_c = message%MPI_VAL
        ierror_c = MPIR_Imrecv_cdesc(buf, count_c, datatype_c, message_c, request_c)
        message%MPI_VAL = message_c
        request%MPI_VAL = request_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Imrecv_f08ts
