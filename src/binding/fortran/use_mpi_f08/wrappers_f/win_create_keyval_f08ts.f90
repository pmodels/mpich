!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_create_keyval_f08(win_copy_attr_fn, win_delete_attr_fn, win_keyval, &
    extra_state, ierror)
    use, intrinsic :: iso_c_binding, only : c_funloc
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_f08, only : MPI_Win_copy_attr_function
    use :: mpi_f08, only : MPI_Win_delete_attr_function
    use :: mpi_c_interface, only : MPIR_Win_create_keyval_c
    use :: mpi_c_interface, only : MPIR_Keyval_set_proxy, MPIR_Win_copy_attr_f08_proxy, MPIR_Win_delete_attr_f08_proxy

    implicit none

    procedure(MPI_Win_copy_attr_function) :: win_copy_attr_fn
    procedure(MPI_Win_delete_attr_function) :: win_delete_attr_fn
    integer, intent(out) :: win_keyval
    integer(MPI_ADDRESS_KIND), intent(in) :: extra_state
    integer, optional, intent(out) :: ierror

    type(c_funptr) :: win_copy_attr_fn_c
    type(c_funptr) :: win_delete_attr_fn_c
    integer(c_int) :: win_keyval_c
    integer(c_int) :: ierror_c

    win_copy_attr_fn_c = c_funloc(win_copy_attr_fn)
    win_delete_attr_fn_c = c_funloc(win_delete_attr_fn)

    ierror_c = MPIR_Win_create_keyval_c(win_copy_attr_fn_c, win_delete_attr_fn_c, win_keyval_c, extra_state)

    call MPIR_Keyval_set_proxy(win_keyval_c, c_funloc(MPIR_Win_copy_attr_f08_proxy), c_funloc(MPIR_Win_delete_attr_f08_proxy))
    win_keyval = win_keyval_c
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_create_keyval_f08
