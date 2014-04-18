!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Unpack_external_f08ts(datarep, inbuf, insize, position, outbuf, outcount, &
    datatype, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Unpack_external_cdesc
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: datarep
    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer(MPI_ADDRESS_KIND), intent(in) :: insize
    integer(MPI_ADDRESS_KIND), intent(inout) :: position
    integer, intent(in) :: outcount
    type(MPI_Datatype), intent(in) :: datatype
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: datarep_c(len_trim(datarep)+1)
    integer(MPI_ADDRESS_KIND) :: insize_c
    integer(MPI_ADDRESS_KIND) :: position_c
    integer(c_int) :: outcount_c
    integer(c_Datatype) :: datatype_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(datarep, datarep_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Unpack_external_cdesc(datarep_c, inbuf, insize, position, outbuf, outcount, datatype%MPI_VAL)
    else
        insize_c = insize
        position_c = position
        outcount_c = outcount
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Unpack_external_cdesc(datarep_c, inbuf, insize_c, position_c, outbuf, outcount_c, &
            datatype_c)
        position = position_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Unpack_external_f08ts
