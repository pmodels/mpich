!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Reduce_local_f08ts(inbuf, inoutbuf, count, datatype, op, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Op
    use :: mpi_c_interface, only : c_Datatype, c_Op
    use :: mpi_c_interface, only : MPIR_Reduce_local_cdesc

    implicit none

    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: inoutbuf
    integer, intent(in) :: count
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Op), intent(in) :: op
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_Datatype) :: datatype_c
    integer(c_Op) :: op_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Reduce_local_cdesc(inbuf, inoutbuf, count, datatype%MPI_VAL, op%MPI_VAL)
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        op_c = op%MPI_VAL
        ierror_c = MPIR_Reduce_local_cdesc(inbuf, inoutbuf, count_c, datatype_c, op_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Reduce_local_f08ts
