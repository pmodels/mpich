!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
module mpi_f08_callbacks

! MPI3.0, A.1.3,  p. 678

public :: MPI_COMM_DUP_FN
public :: MPI_COMM_NULL_COPY_FN
public :: MPI_COMM_NULL_DELETE_FN
public :: MPI_TYPE_DUP_FN
public :: MPI_TYPE_NULL_COPY_FN
public :: MPI_TYPE_NULL_DELETE_FN
public :: MPI_WIN_DUP_FN
public :: MPI_WIN_NULL_COPY_FN
public :: MPI_WIN_NULL_DELETE_FN
public :: MPI_CONVERSION_FN_NULL

abstract interface

subroutine MPI_User_function(invec, inoutvec, len, datatype)
    use, intrinsic :: iso_c_binding, only : c_ptr
    use mpi_f08_types, only : MPI_Datatype
    implicit none
    type(c_ptr), value :: invec, inoutvec
    integer :: len
    type(MPI_Datatype) :: datatype
end subroutine

subroutine MPI_Comm_copy_attr_function(oldcomm,comm_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Comm
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Comm) :: oldcomm
    integer :: comm_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag
end subroutine

subroutine MPI_Comm_delete_attr_function(comm,comm_keyval, &
       attribute_val, extra_state, ierror)
    use mpi_f08_types, only : MPI_Comm
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Comm) :: comm
    integer :: comm_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: attribute_val, extra_state
end subroutine

subroutine MPI_Win_copy_attr_function(oldwin,win_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Win
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Win) :: oldwin
    integer :: win_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag
end subroutine

subroutine MPI_Win_delete_attr_function(win,win_keyval,attribute_val, &
       extra_state,ierror)
    use mpi_f08_types, only : MPI_Win
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Win) :: win
    integer :: win_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: attribute_val, extra_state
end subroutine

subroutine MPI_Type_copy_attr_function(oldtype,type_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Datatype) :: oldtype
    integer :: type_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag
end subroutine

subroutine MPI_Type_delete_attr_function(datatype,type_keyval, &
       attribute_val,extra_state,ierror)
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Datatype) :: datatype
    integer :: type_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: attribute_val, extra_state
end subroutine

subroutine MPI_Comm_errhandler_function(comm,error_code)
    use mpi_f08_types, only : MPI_Comm
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Comm) :: comm
    integer :: error_code
end subroutine

subroutine MPI_Win_errhandler_function(win, error_code)
    use mpi_f08_types, only : MPI_Win
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Win) :: win
    integer :: error_code
end subroutine

subroutine MPI_File_errhandler_function(file, error_code)
    use mpi_f08_types, only : MPI_File
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_File) :: file
    integer :: error_code
end subroutine

subroutine MPI_Grequest_query_function(extra_state,status,ierror)
    use mpi_f08_types, only : MPI_Status
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Status) :: status
    integer :: ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state
end subroutine

subroutine MPI_Grequest_free_function(extra_state,ierror)
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer :: ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state
end subroutine

subroutine MPI_Grequest_cancel_function(extra_state,complete,ierror)
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(kind=MPI_ADDRESS_KIND) :: extra_state
    logical :: complete
    integer :: ierror
end subroutine

subroutine MPI_Datarep_extent_function(datatype, extent, extra_state, ierror)
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(MPI_Datatype) :: datatype
    integer :: ierror
    integer(kind=MPI_ADDRESS_KIND) :: extent, extra_state
end subroutine

subroutine MPI_Datarep_conversion_function(userbuf, datatype, count, &
       filebuf, position, extra_state, ierror)
    use, intrinsic :: iso_c_binding, only : c_ptr
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_OFFSET_KIND, MPI_ADDRESS_KIND
    implicit none
    type(c_ptr), value :: userbuf, filebuf
    type(MPI_Datatype) :: datatype
    integer :: count, ierror
    integer(kind=MPI_OFFSET_KIND) :: position
    integer(kind=MPI_ADDRESS_KIND) :: extra_state
end subroutine

end interface

contains

! See p.269, MPI 3.0
subroutine MPI_COMM_DUP_FN(oldcomm,comm_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Comm
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Comm) :: oldcomm
    integer :: comm_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag

    flag = .true.
    attribute_val_out = attribute_val_in
    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_COMM_NULL_COPY_FN(oldcomm,comm_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Comm
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Comm) :: oldcomm
    integer :: comm_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag

    flag = .false.
    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_COMM_NULL_DELETE_FN(comm,comm_keyval, &
       attribute_val, extra_state, ierror)
    use mpi_f08_types, only : MPI_Comm
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Comm) :: comm
    integer :: comm_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: attribute_val, extra_state

    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_TYPE_DUP_FN(oldtype,type_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Datatype) :: oldtype
    integer :: type_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag

    flag = .true.
    attribute_val_out = attribute_val_in
    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_TYPE_NULL_COPY_FN(oldtype,type_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Datatype) :: oldtype
    integer :: type_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag

    flag = .false.
    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_TYPE_NULL_DELETE_FN(datatype,type_keyval, &
       attribute_val, extra_state, ierror)
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Datatype) :: datatype
    integer :: type_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: attribute_val, extra_state

    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_WIN_DUP_FN(oldwin,win_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Win
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Win) :: oldwin
    integer :: win_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag

    flag = .true.
    attribute_val_out = attribute_val_in
    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_WIN_NULL_COPY_FN(oldwin,win_keyval,extra_state, &
       attribute_val_in,attribute_val_out,flag,ierror)
    use mpi_f08_types, only : MPI_Win
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Win) :: oldwin
    integer :: win_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: extra_state, attribute_val_in, attribute_val_out
    logical :: flag

    flag = .false.
    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_WIN_NULL_DELETE_FN(win,win_keyval, &
       attribute_val, extra_state, ierror)
    use mpi_f08_types, only : MPI_Win
    use mpi_f08_compile_constants, only : MPI_ADDRESS_KIND, MPI_SUCCESS
    implicit none
    type(MPI_Win) :: win
    integer :: win_keyval, ierror
    integer(kind=MPI_ADDRESS_KIND) :: attribute_val, extra_state

    ierror = MPI_SUCCESS
end subroutine

subroutine MPI_CONVERSION_FN_NULL(userbuf, datatype, count, &
       filebuf, position, extra_state, ierror)
    use, intrinsic :: iso_c_binding, only : c_ptr
    use mpi_f08_types, only : MPI_Datatype
    use mpi_f08_compile_constants, only : MPI_OFFSET_KIND, MPI_ADDRESS_KIND
    implicit none
    type(c_ptr), value :: userbuf, filebuf
    type(MPI_Datatype) :: datatype
    integer :: count, ierror
    integer(kind=MPI_OFFSET_KIND) :: position
    integer(kind=MPI_ADDRESS_KIND) :: extra_state
    ! Do nothing
end subroutine

end module mpi_f08_callbacks
