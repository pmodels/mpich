!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Close_port_f08(port_name, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface, only : MPIR_Close_port_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: port_name
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: port_name_c(len_trim(port_name)+1)
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(port_name, port_name_c)

    ierror_c = MPIR_Close_port_c(port_name_c)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Close_port_f08
