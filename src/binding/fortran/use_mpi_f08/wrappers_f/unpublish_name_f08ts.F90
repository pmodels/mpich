!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Unpublish_name_f08(service_name, info, port_name, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Info
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_Unpublish_name_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: service_name
    character(len=*), intent(in) :: port_name
    type(MPI_Info), intent(in) :: info
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: service_name_c(len_trim(service_name)+1)
    character(kind=c_char) :: port_name_c(len_trim(port_name)+1)
    integer(c_Info) :: info_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(service_name, service_name_c)

    call MPIR_Fortran_string_f2c(port_name, port_name_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Unpublish_name_c(service_name_c, info%MPI_VAL, port_name_c)
    else
        info_c = info%MPI_VAL
        ierror_c = MPIR_Unpublish_name_c(service_name_c, info_c, port_name_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Unpublish_name_f08
