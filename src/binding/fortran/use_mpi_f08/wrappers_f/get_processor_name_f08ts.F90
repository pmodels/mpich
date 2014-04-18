!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Get_processor_name_f08(name, resultlen, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_MAX_PROCESSOR_NAME
    use :: mpi_c_interface, only : MPIR_Get_processor_name_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    character(len=MPI_MAX_PROCESSOR_NAME), intent(out) :: name
    integer, intent(out) :: resultlen
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: name_c(MPI_MAX_PROCESSOR_NAME+1)
    integer(c_int) :: resultlen_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Get_processor_name_c(name_c, resultlen)
    else
        ierror_c = MPIR_Get_processor_name_c(name_c, resultlen_c)
        resultlen = resultlen_c
    end if

    call MPIR_Fortran_string_c2f(name_c, name)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Get_processor_name_f08
