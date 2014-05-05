!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Pack_f08ts(inbuf, incount, datatype, outbuf, outsize, position, comm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm
    use :: mpi_c_interface, only : c_Datatype, c_Comm
    use :: mpi_c_interface, only : MPIR_Pack_cdesc

    implicit none

    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer, intent(in) :: incount
    integer, intent(in) :: outsize
    type(MPI_Datatype), intent(in) :: datatype
    integer, intent(inout) :: position
    type(MPI_Comm), intent(in) :: comm
    integer, optional, intent(out) :: ierror

    integer(c_int) :: incount_c
    integer(c_int) :: outsize_c
    integer(c_Datatype) :: datatype_c
    integer(c_int) :: position_c
    integer(c_Comm) :: comm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Pack_cdesc(inbuf, incount, datatype%MPI_VAL, outbuf, outsize, position, comm%MPI_VAL)
    else
        incount_c = incount
        datatype_c = datatype%MPI_VAL
        outsize_c = outsize
        position_c = position
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Pack_cdesc(inbuf, incount_c, datatype_c, outbuf, outsize_c, position_c, comm_c)
        position = position_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Pack_f08ts
