!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Unpack_f08ts(inbuf, insize, position, outbuf, outcount, datatype, comm, &
    ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm
    use :: mpi_c_interface, only : c_Datatype, c_Comm
    use :: mpi_c_interface, only : MPIR_Unpack_cdesc

    implicit none

    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer, intent(in) :: insize
    integer, intent(in) :: outcount
    integer, intent(inout) :: position
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Comm), intent(in) :: comm
    integer, optional, intent(out) :: ierror

    integer(c_int) :: insize_c
    integer(c_int) :: outcount_c
    integer(c_int) :: position_c
    integer(c_Datatype) :: datatype_c
    integer(c_Comm) :: comm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Unpack_cdesc(inbuf, insize, position, outbuf, outcount, datatype%MPI_VAL, comm%MPI_VAL)
    else
        insize_c = insize
        position_c = position
        outcount_c = outcount
        datatype_c = datatype%MPI_VAL
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Unpack_cdesc(inbuf, insize_c, position_c, outbuf, outcount_c, datatype_c, comm_c)
        position = position_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Unpack_f08ts
