!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!

module mpi_c_interface_glue

use, intrinsic :: iso_c_binding, only : c_char, C_NULL_CHAR

implicit none

public :: MPIR_Fortran_string_f2c
public :: MPIR_Fortran_string_c2f

public :: MPIR_Comm_copy_attr_f08_proxy
public :: MPIR_Comm_delete_attr_f08_proxy
public :: MPIR_Type_copy_attr_f08_proxy
public :: MPIR_Type_delete_attr_f08_proxy
public :: MPIR_Win_copy_attr_f08_proxy
public :: MPIR_Win_delete_attr_f08_proxy
public :: MPIR_Keyval_set_proxy
public :: MPIR_Grequest_set_lang_fortran

! Bind to C's enum MPIR_AttrType in mpi_attr.h
enum, bind(C)
    enumerator :: MPIR_ATTR_PTR  = 0
    enumerator :: MPIR_ATTR_AINT = 1
    enumerator :: MPIR_ATTR_INT  = 3
end enum

interface

subroutine MPIR_Keyval_set_proxy(keyval, attr_copy_proxy, attr_delete_proxy) bind(C, name="MPIR_Keyval_set_proxy")
    use :: iso_c_binding, only : c_int, c_funptr
    integer(c_int), value, intent(in) :: keyval
    type(c_funptr), value, intent(in) :: attr_copy_proxy, attr_delete_proxy
    ! The subroutine is implemented in attrutil.c on the C side
end subroutine MPIR_Keyval_set_proxy

! Just need to tag the lang is Fortran, so it is fine to bind to *_lang_f77
subroutine MPIR_Grequest_set_lang_fortran(request) bind(C, name="MPIR_Grequest_set_lang_f77")
    use :: mpi_c_interface_types, only : c_Request
    integer(c_Request), value, intent(in) :: request
    ! The subroutine is implemented in mpir_request.c on the C side
end subroutine MPIR_Grequest_set_lang_fortran

end interface

contains

! Copy Fortran string to C charater array, assuming the C array is one-char
! longer for the terminating null char.
! fstring : the Fortran input string
! cstring : the C output string (with memory already allocated)
subroutine MPIR_Fortran_string_f2c(fstring, cstring)
    implicit none
    character(len=*), intent(in) :: fstring
    character(kind=c_char), intent(out) :: cstring(:)
    integer :: i, j
    logical :: met_non_blank

    ! Trim the leading and trailing blank characters
    j = 1
    met_non_blank = .false.
    do i = 1, len_trim(fstring)
        if (met_non_blank) then
            cstring(j) = fstring(i:i)
            j = j + 1
        else if (fstring(i:i) /= ' ') then
            met_non_blank = .true.
            cstring(j) = fstring(i:i)
            j = j + 1
        end if
    end do

    cstring(j) = C_NULL_CHAR
end subroutine MPIR_Fortran_string_f2c

! Copy C charater array to Fortran string
subroutine MPIR_Fortran_string_c2f(cstring, fstring)
    implicit none
    character(kind=c_char), intent(in) :: cstring(:)
    character(len=*), intent(out) :: fstring
    integer :: i, j, length

    i = 1
    do while (cstring(i) /= C_NULL_CHAR)
        fstring(i:i) = cstring(i)
        i = i + 1
    end do

    ! Zero out the trailing characters
    length = len(fstring)
    do j = i, length
        fstring(j:j) = ' '
    end do
end subroutine MPIR_Fortran_string_c2f

function MPIR_Comm_copy_attr_f08_proxy (user_function, oldcomm, comm_keyval, extra_state, &
        attr_type, attribute_val_in, attribute_val_out, flag) result(ierror)

    use :: iso_c_binding, only : c_int, c_intptr_t
    use :: mpi_f08, only : MPI_ADDRESS_KIND, MPI_Comm, MPI_Comm_copy_attr_function
    use :: mpi_c_interface_types, only : c_Comm

    implicit none

    procedure (MPI_Comm_copy_attr_function)          :: user_function
    integer(c_Comm), value, intent(in)               :: oldcomm
    integer(c_int), value, intent(in)                :: comm_keyval
    integer(c_intptr_t), value, intent(in)           :: extra_state
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type ! Only used in C proxy
    integer(c_intptr_t), value, intent(in)           :: attribute_val_in
    integer(c_intptr_t), intent(out)                 :: attribute_val_out
    integer(c_int), intent(out)                      :: flag
    integer(c_int)                                   :: ierror

    type(MPI_Comm)            :: oldcomm_f
    integer                   :: comm_keyval_f
    integer(MPI_ADDRESS_KIND) :: extra_state_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_in_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_out_f
    logical                   :: flag_f
    integer                   :: ierror_f

    oldcomm_f%MPI_VAL   = oldcomm
    comm_keyval_f       = comm_keyval
    attribute_val_in_f  = attribute_val_in
    extra_state_f       = extra_state

    call user_function(oldcomm_f, comm_keyval_f, extra_state_f, attribute_val_in_f, attribute_val_out_f, flag_f, ierror_f)

    attribute_val_out = attribute_val_out_f
    flag = merge(1, 0, flag_f)
    ierror = ierror_f

end function MPIR_Comm_copy_attr_f08_proxy

function MPIR_Comm_delete_attr_f08_proxy (user_function, comm, comm_keyval, attr_type, &
        attribute_val, extra_state) result(ierror)
    use :: iso_c_binding, only : c_int, c_intptr_t
    use :: mpi_f08, only : MPI_ADDRESS_KIND, MPI_Comm, MPI_Comm_delete_attr_function
    use :: mpi_c_interface_types, only : c_Comm

    implicit none

    procedure (MPI_Comm_delete_attr_function)        :: user_function
    integer(c_Comm), value, intent(in)               :: comm
    integer(c_int), value, intent(in)                :: comm_keyval
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type ! Only used in C proxy
    integer(c_intptr_t), value, intent(in)           :: attribute_val
    integer(c_intptr_t), value, intent(in)           :: extra_state
    integer(c_int)                                   :: ierror

    type(MPI_Comm)            :: comm_f
    integer                   :: comm_keyval_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_f
    integer(MPI_ADDRESS_KIND) :: extra_state_f
    integer                   :: ierror_f

    comm_f%MPI_VAL  = comm
    comm_keyval_f   = comm_keyval
    attribute_val_f = attribute_val
    extra_state_f   = extra_state

    call user_function(comm_f, comm_keyval_f, attribute_val_f, extra_state_f, ierror_f)

    ierror = ierror_f

end function MPIR_Comm_delete_attr_f08_proxy

function MPIR_Type_copy_attr_f08_proxy (user_function, oldtype, type_keyval, extra_state, &
        attr_type, attribute_val_in, attribute_val_out, flag) result(ierror)

    use :: iso_c_binding, only : c_int, c_intptr_t
    use :: mpi_f08, only : MPI_ADDRESS_KIND, MPI_Datatype, MPI_Type_copy_attr_function
    use :: mpi_c_interface_types, only : c_Datatype

    implicit none

    procedure (MPI_Type_copy_attr_function)          :: user_function
    integer(c_Datatype), value, intent(in)           :: oldtype
    integer(c_int), value, intent(in)                :: type_keyval
    integer(c_intptr_t), value, intent(in)           :: extra_state
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type
    integer(c_intptr_t), value, intent(in)           :: attribute_val_in
    integer(c_intptr_t), intent(out)                 :: attribute_val_out
    integer(c_int), intent(out)                      :: flag
    integer(c_int)                                   :: ierror

    type(MPI_Datatype)        :: oldtype_f
    integer                   :: type_keyval_f
    integer(MPI_ADDRESS_KIND) :: extra_state_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_in_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_out_f
    logical                   :: flag_f
    integer                   :: ierror_f

    oldtype_f%MPI_VAL   = oldtype
    type_keyval_f       = type_keyval
    attribute_val_in_f  = attribute_val_in
    extra_state_f       = extra_state

    call user_function(oldtype_f, type_keyval_f, extra_state_f, attribute_val_in_f, attribute_val_out_f, flag_f, ierror_f)

    attribute_val_out = attribute_val_out_f
    flag = merge(1, 0, flag_f)
    ierror = ierror_f

end function MPIR_Type_copy_attr_f08_proxy

function MPIR_Type_delete_attr_f08_proxy (user_function, type, type_keyval, attr_type, &
        attribute_val, extra_state) result(ierror)
    use :: iso_c_binding, only : c_int, c_intptr_t
    use :: mpi_f08, only : MPI_ADDRESS_KIND, MPI_Datatype, MPI_Type_delete_attr_function
    use :: mpi_c_interface_types, only : c_Datatype

    implicit none

    procedure (MPI_Type_delete_attr_function)        :: user_function
    integer(c_Datatype), value, intent(in)           :: type
    integer(c_int), value, intent(in)                :: type_keyval
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type ! Only used in C proxy
    integer(c_intptr_t), value, intent(in)           :: attribute_val
    integer(c_intptr_t), value, intent(in)           :: extra_state
    integer(c_int)                                   :: ierror

    type(MPI_Datatype)        :: type_f
    integer                   :: type_keyval_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_f
    integer(MPI_ADDRESS_KIND) :: extra_state_f
    integer                   :: ierror_f

    type_f%MPI_VAL  = type
    type_keyval_f   = type_keyval
    attribute_val_f = attribute_val
    extra_state_f   = extra_state

    call user_function(type_f, type_keyval_f, attribute_val_f, extra_state_f, ierror_f)

    ierror = ierror_f

end function MPIR_Type_delete_attr_f08_proxy

function MPIR_Win_copy_attr_f08_proxy (user_function, oldwin, win_keyval, extra_state, &
        attr_type, attribute_val_in, attribute_val_out, flag) result(ierror)

    use :: iso_c_binding, only : c_int, c_intptr_t
    use :: mpi_f08, only : MPI_ADDRESS_KIND, MPI_Win, MPI_Win_copy_attr_function
    use :: mpi_c_interface_types, only : c_Win

    implicit none

    procedure (MPI_Win_copy_attr_function)           :: user_function
    integer(c_Win), value, intent(in)                :: oldwin
    integer(c_int), value, intent(in)                :: win_keyval
    integer(c_intptr_t), value, intent(in)           :: extra_state
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type ! Only used in C proxy
    integer(c_intptr_t), value, intent(in)           :: attribute_val_in
    integer(c_intptr_t), intent(out)                 :: attribute_val_out
    integer(c_int), intent(out)                      :: flag
    integer(c_int)                                   :: ierror

    type(MPI_Win)             :: oldwin_f
    integer                   :: win_keyval_f
    integer(MPI_ADDRESS_KIND) :: extra_state_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_in_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_out_f
    logical                   :: flag_f
    integer                   :: ierror_f

    oldwin_f%MPI_VAL   = oldwin
    win_keyval_f       = win_keyval
    attribute_val_in_f  = attribute_val_in
    extra_state_f       = extra_state

    call user_function(oldwin_f, win_keyval_f, extra_state_f, attribute_val_in_f, attribute_val_out_f, flag_f, ierror_f)

    attribute_val_out = attribute_val_out_f
    flag = merge(1, 0, flag_f)
    ierror = ierror_f

end function MPIR_Win_copy_attr_f08_proxy

function MPIR_Win_delete_attr_f08_proxy (user_function, win, win_keyval, attr_type, &
        attribute_val, extra_state) result(ierror)
    use :: iso_c_binding, only : c_int, c_intptr_t
    use :: mpi_f08, only : MPI_ADDRESS_KIND, MPI_Win, MPI_Win_delete_attr_function
    use :: mpi_c_interface_types, only : c_Win

    implicit none

    procedure (MPI_Win_delete_attr_function)         :: user_function
    integer(c_Win), value, intent(in)                :: win
    integer(c_int), value, intent(in)                :: win_keyval
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type ! Only used in C proxy
    integer(c_intptr_t), value, intent(in)           :: attribute_val
    integer(c_intptr_t), value, intent(in)           :: extra_state
    integer(c_int)                                   :: ierror

    type(MPI_Win)             :: win_f
    integer                   :: win_keyval_f
    integer(MPI_ADDRESS_KIND) :: attribute_val_f
    integer(MPI_ADDRESS_KIND) :: extra_state_f
    integer                   :: ierror_f

    win_f%MPI_VAL  = win
    win_keyval_f   = win_keyval
    attribute_val_f = attribute_val
    extra_state_f   = extra_state

    call user_function(win_f, win_keyval_f, attribute_val_f, extra_state_f, ierror_f)

    ierror = ierror_f

end function MPIR_Win_delete_attr_f08_proxy

end module mpi_c_interface_glue
