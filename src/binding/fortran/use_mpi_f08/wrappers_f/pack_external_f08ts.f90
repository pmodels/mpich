!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Pack_external_f08ts(datarep, inbuf, incount, datatype, outbuf, outsize, &
    position, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Pack_external_cdesc
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: datarep
    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer, intent(in) :: incount
    type(MPI_Datatype), intent(in) :: datatype
    integer(MPI_ADDRESS_KIND), intent(in) :: outsize
    integer(MPI_ADDRESS_KIND), intent(inout) :: position
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: datarep_c(len_trim(datarep)+1)
    integer(c_int) :: incount_c
    integer(c_Datatype) :: datatype_c
    integer(MPI_ADDRESS_KIND) :: outsize_c
    integer(MPI_ADDRESS_KIND) :: position_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(datarep, datarep_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Pack_external_cdesc(datarep_c, inbuf, incount, datatype%MPI_VAL, outbuf, outsize, &
            position)
    else
        incount_c = incount
        datatype_c = datatype%MPI_VAL
        outsize_c = outsize
        position_c = position
        ierror_c = MPIR_Pack_external_cdesc(datarep_c, inbuf, incount_c, datatype_c, outbuf, outsize_c, &
            position_c)
        position = position_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Pack_external_f08ts
