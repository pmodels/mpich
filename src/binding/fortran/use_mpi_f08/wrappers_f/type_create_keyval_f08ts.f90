!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_create_keyval_f08(type_copy_attr_fn, type_delete_attr_fn, type_keyval, &
    extra_state, ierror)
    use, intrinsic :: iso_c_binding, only : c_funloc
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_f08, only : MPI_Type_copy_attr_function
    use :: mpi_f08, only : MPI_Type_delete_attr_function
    use :: mpi_c_interface, only : MPIR_Type_create_keyval_c
    use :: mpi_c_interface, only : MPIR_Keyval_set_proxy, MPIR_Type_copy_attr_f08_proxy, MPIR_Type_delete_attr_f08_proxy

    implicit none

    procedure(MPI_Type_copy_attr_function) :: type_copy_attr_fn
    procedure(MPI_Type_delete_attr_function) :: type_delete_attr_fn
    integer, intent(out) :: type_keyval
    integer(MPI_ADDRESS_KIND), intent(in) :: extra_state
    integer, optional, intent(out) :: ierror

    type(c_funptr) :: type_copy_attr_fn_c
    type(c_funptr) :: type_delete_attr_fn_c
    integer(c_int) :: type_keyval_c
    integer(c_int) :: ierror_c

    type_copy_attr_fn_c = c_funloc(type_copy_attr_fn)
    type_delete_attr_fn_c = c_funloc(type_delete_attr_fn)

    ierror_c = MPIR_Type_create_keyval_c(type_copy_attr_fn_c, type_delete_attr_fn_c, type_keyval_c, extra_state)

    call MPIR_Keyval_set_proxy(type_keyval_c, c_funloc(MPIR_Type_copy_attr_f08_proxy), c_funloc(MPIR_Type_delete_attr_f08_proxy))
    type_keyval = type_keyval_c
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_create_keyval_f08
