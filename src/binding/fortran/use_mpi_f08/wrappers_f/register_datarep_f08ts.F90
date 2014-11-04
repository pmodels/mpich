!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Register_datarep_f08(datarep, read_conversion_fn, write_conversion_fn, &
    dtype_file_extent_fn, extra_state, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use, intrinsic :: iso_c_binding, only : c_funloc, c_funptr, c_associated, C_NULL_FUNPTR
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_f08, only : MPI_CONVERSION_FN_NULL
    use :: mpi_f08, only : MPI_Datarep_conversion_function
    use :: mpi_f08, only : MPI_Datarep_extent_function
    use :: mpi_c_interface, only : MPIR_Register_datarep_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: datarep
    procedure(MPI_Datarep_conversion_function) :: read_conversion_fn
    procedure(MPI_Datarep_conversion_function) :: write_conversion_fn
    procedure(MPI_Datarep_extent_function) :: dtype_file_extent_fn
    integer(MPI_ADDRESS_KIND), intent(in) :: extra_state
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: datarep_c(len_trim(datarep)+1)
    type(c_funptr) :: read_conversion_fn_c
    type(c_funptr) :: write_conversion_fn_c
    type(c_funptr) :: dtype_file_extent_fn_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(datarep, datarep_c)

    read_conversion_fn_c = c_funloc(read_conversion_fn)
    write_conversion_fn_c = c_funloc(write_conversion_fn)
    dtype_file_extent_fn_c = c_funloc(dtype_file_extent_fn)

    if (c_associated(read_conversion_fn_c, c_funloc(MPI_CONVERSION_FN_NULL))) then
        read_conversion_fn_c = C_NULL_FUNPTR
    end if

    if (c_associated(write_conversion_fn_c, c_funloc(MPI_CONVERSION_FN_NULL))) then
        write_conversion_fn_c = C_NULL_FUNPTR
    end if

    ierror_c = MPIR_Register_datarep_c(datarep_c, read_conversion_fn_c, write_conversion_fn_c, &
                                       dtype_file_extent_fn_c, extra_state)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Register_datarep_f08
