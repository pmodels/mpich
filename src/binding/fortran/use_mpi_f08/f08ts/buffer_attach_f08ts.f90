subroutine MPI_Buffer_attach_f08ts(buffer, size, ierror)
     use,intrinsic :: iso_c_binding, only: C_int
     use :: mpi_c_interface,        only: MPIR_Buffer_attach_cdesc
     type(*), DIMENSION(..), ASYNCHRONOUS :: buffer  !!! CORRECT
     integer, intent(in) :: size
     integer, optional, intent(out) :: ierror
     integer(C_int)  :: size_c
     integer(C_int)  :: res

     size_c = size
     res = MPIR_Buffer_attach_cdesc (buffer, size_c)
     if (present(ierror)) ierror = res

end subroutine MPI_Buffer_attach_f08ts

