!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Sendrecv_f08ts(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, &
    recvcount, recvtype, source, recvtag, comm, status, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm, MPI_Status
    use :: mpi_f08, only : MPI_STATUS_IGNORE, MPIR_C_MPI_STATUS_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Datatype, c_Comm
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Sendrecv_cdesc

    implicit none

    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer, intent(in) :: sendcount
    integer, intent(in) :: dest
    integer, intent(in) :: sendtag
    integer, intent(in) :: recvcount
    integer, intent(in) :: source
    integer, intent(in) :: recvtag
    type(MPI_Datatype), intent(in) :: sendtype
    type(MPI_Datatype), intent(in) :: recvtype
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Status), target :: status
    integer, optional, intent(out) :: ierror

    integer(c_int) :: sendcount_c
    integer(c_int) :: dest_c
    integer(c_int) :: sendtag_c
    integer(c_int) :: recvcount_c
    integer(c_int) :: source_c
    integer(c_int) :: recvtag_c
    integer(c_Datatype) :: sendtype_c
    integer(c_Datatype) :: recvtype_c
    integer(c_Comm) :: comm_c
    type(c_Status), target :: status_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Sendrecv_cdesc(sendbuf, sendcount, sendtype%MPI_VAL, dest, sendtag, recvbuf, recvcount, &
                recvtype%MPI_VAL, source, recvtag, comm%MPI_VAL, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Sendrecv_cdesc(sendbuf, sendcount, sendtype%MPI_VAL, dest, sendtag, recvbuf, recvcount, &
                recvtype%MPI_VAL, source, recvtag, comm%MPI_VAL, c_loc(status))
        end if
    else
        sendcount_c = sendcount
        sendtype_c = sendtype%MPI_VAL
        dest_c = dest
        sendtag_c = sendtag
        recvcount_c = recvcount
        recvtype_c = recvtype%MPI_VAL
        source_c = source
        recvtag_c = recvtag
        comm_c = comm%MPI_VAL
        if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
            ierror_c = MPIR_Sendrecv_cdesc(sendbuf, sendcount_c, sendtype_c, dest_c, sendtag_c, recvbuf, &
                recvcount_c, recvtype_c, source_c, recvtag_c, comm_c, MPIR_C_MPI_STATUS_IGNORE)
        else
            ierror_c = MPIR_Sendrecv_cdesc(sendbuf, sendcount_c, sendtype_c, dest_c, sendtag_c, recvbuf, &
                recvcount_c, recvtype_c, source_c, recvtag_c, comm_c, c_loc(status_c))
            status = status_c
        end if
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Sendrecv_f08ts
