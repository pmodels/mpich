!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Grequest_start_f08(query_fn, free_fn, cancel_fn, extra_state, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use, intrinsic :: iso_c_binding, only : c_funloc, c_funptr
    use :: mpi_f08, only : MPI_Request
    use :: mpi_f08, only : MPI_SUCCESS, MPI_ADDRESS_KIND
    use :: mpi_f08, only : MPI_Grequest_query_function
    use :: mpi_f08, only : MPI_Grequest_free_function
    use :: mpi_f08, only : MPI_Grequest_cancel_function
    use :: mpi_c_interface, only : c_Request
    use :: mpi_c_interface, only : MPIR_Grequest_start_c
    use :: mpi_c_interface, only : MPIR_Grequest_set_lang_fortran

    implicit none

    procedure(MPI_Grequest_query_function) :: query_fn
    procedure(MPI_Grequest_free_function) :: free_fn
    procedure(MPI_Grequest_cancel_function) :: cancel_fn
    integer(MPI_ADDRESS_KIND), intent(in) :: extra_state
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    type(c_funptr) :: query_fn_c
    type(c_funptr) :: free_fn_c
    type(c_funptr) :: cancel_fn_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    query_fn_c = c_funloc(query_fn)
    free_fn_c = c_funloc(free_fn)
    cancel_fn_c = c_funloc(cancel_fn)

    ierror_c = MPIR_Grequest_start_c(query_fn_c, free_fn_c, cancel_fn_c, extra_state, request_c)

    if (ierror_c == MPI_SUCCESS) then
        call MPIR_Grequest_set_lang_fortran(request_c)
    end if

    request%MPI_VAL = request_c
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Grequest_start_f08
