!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Bsend_f08ts(buf, count, datatype, dest, tag, comm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Comm
    use :: mpi_c_interface, only : MPIR_Bsend_cdesc

    implicit none

    type(*), dimension(..), intent(in) :: buf
    integer, intent(in) :: count
    integer, intent(in) :: dest
    integer, intent(in) :: tag
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Comm), intent(in) :: comm
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_int) :: dest_c
    integer(c_int) :: tag_c
    integer(c_Datatype) :: datatype_c
    integer(c_Comm) :: comm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Bsend_cdesc(buf, count, datatype%MPI_VAL, dest, tag, comm%MPI_VAL)
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        dest_c = dest
        tag_c = tag
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Bsend_cdesc(buf, count_c, datatype_c, dest_c, tag_c, comm_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Bsend_f08ts
