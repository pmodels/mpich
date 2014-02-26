module mpi_f08_callbacks

! MPI3.0, A.1.3,  p. 678

abstract interface

subroutine MPI_User_function(invec, inoutvec, len, datatype)
    USE, intrinsic :: ISO_C_BINDING, only : C_PTR
    USE mpi_f08_types, only : MPI_Datatype
    implicit none
    type(C_PTR), value :: invec, inoutvec
    integer :: len
    type(MPI_Datatype) :: datatype
end subroutine

subroutine MPI_Comm_copy_attr_function(oldcomm,comm_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    USE mpi_f08_types, only : MPI_Comm
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Comm) :: oldcomm
    integer :: comm_keyval, ierror
    integer(KIND=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag
end subroutine

subroutine MPI_Comm_delete_attr_function(comm,comm_keyval, &
       attribute_val, extra_state, ierror)
    USE mpi_f08_types, only : MPI_Comm
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Comm) :: comm
    integer :: comm_keyval, ierror
    integer(KIND=MPI_ADDRESS_KIND) :: attribute_val, extra_state
end subroutine

subroutine MPI_Win_copy_attr_function(oldwin,win_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    USE mpi_f08_types, only : MPI_Win
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Win) :: oldwin
    integer :: win_keyval, ierror
    integer(KIND=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag
end subroutine

subroutine MPI_Win_delete_attr_function(win,win_keyval,attribute_val, &
       extra_state,ierror)
    USE mpi_f08_types, only : MPI_Win
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Win) :: win
    integer :: win_keyval, ierror
    integer(KIND=MPI_ADDRESS_KIND) :: attribute_val, extra_state
end subroutine

subroutine MPI_Type_copy_attr_function(oldtype,type_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    USE mpi_f08_types, only : MPI_Datatype
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Datatype) :: oldtype
    integer :: type_keyval, ierror
    integer(KIND=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag
end subroutine

subroutine MPI_Type_delete_attr_function(datatype,type_keyval, &
       attribute_val,extra_state,ierror)
    USE mpi_f08_types, only : MPI_Datatype
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Datatype) :: datatype
    integer :: type_keyval, ierror
    integer(KIND=MPI_ADDRESS_KIND) :: attribute_val, extra_state
end subroutine

subroutine MPI_Comm_errhandler_function(comm,error_code)
    USE mpi_f08_types, only : MPI_Comm
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Comm) :: comm
    integer :: error_code
end subroutine

subroutine MPI_Win_errhandler_function(win, error_code)
    USE mpi_f08_types, only : MPI_Win
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Win) :: win
    integer :: error_code
end subroutine

subroutine MPI_File_errhandler_function(file, error_code)
    USE mpi_f08_types, only : MPI_File
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_File) :: file
    integer :: error_code
end subroutine

    subroutine MPI_Grequest_query_function(extra_state,status,ierror)
    USE mpi_f08_types, only : MPI_Status
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Status) :: status
    integer :: ierror
    integer(KIND=MPI_ADDRESS_KIND) :: extra_state
end subroutine

subroutine MPI_Grequest_free_function(extra_state,ierror)
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer :: ierror
    integer(KIND=MPI_ADDRESS_KIND) :: extra_state
end subroutine

subroutine MPI_Grequest_cancel_function(extra_state,complete,ierror)
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(KIND=MPI_ADDRESS_KIND) :: extra_state
    logical :: complete
    integer :: ierror
end subroutine

subroutine MPI_Datarep_extent_function(datatype, extent, extra_state, ierror)
    USE mpi_f08_types, only : MPI_Datatype
    USE mpi_f08_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Datatype) :: datatype
    integer :: ierror
    integer(KIND=MPI_ADDRESS_KIND) :: extent, extra_state
end subroutine

subroutine MPI_Datarep_conversion_function(userbuf, datatype, count, &
       filebuf, position, extra_state, ierror)
    USE mpi_f08_types, only : MPI_Datatype
    USE mpi_f08_constants, only : MPI_OFFSET_KIND, MPI_ADDRESS_KIND
    USE, intrinsic :: ISO_C_BINDING, only : C_PTR
    implicit none
    type(C_PTR), value :: userbuf, filebuf
    type(MPI_Datatype) :: datatype
    integer :: count, ierror
    integer(KIND=MPI_OFFSET_KIND) :: position
    integer(KIND=MPI_ADDRESS_KIND) :: extra_state
end subroutine

end interface

end module mpi_f08_callbacks
