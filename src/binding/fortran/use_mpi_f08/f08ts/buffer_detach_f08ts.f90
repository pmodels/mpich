subroutine MPI_Buffer_detach_f08 (buffer_addr, size, ierror)
     use,intrinsic :: iso_c_binding, only: c_int, c_ptr
     use :: mpi_c_interface,        only: MPIR_Buffer_detach_c
     type(c_ptr) :: buffer_addr
     integer,intent(out) :: size
     integer,intent(out),optional :: ierror
     integer(c_int) :: size_c
     integer(c_int) :: res

     res = MPIR_Buffer_detach_c (buffer_addr, size_c)
     size = size_c
     if (present(ierror)) ierror = res

end subroutine MPI_Buffer_detach_f08
