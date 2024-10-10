!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

module mpi_c_interface_glue

use, intrinsic :: iso_c_binding, only : c_char, C_NULL_CHAR

implicit none

public :: MPIR_Fortran_string_f2c
public :: MPIR_Fortran_string_c2f

public :: MPIR_Grequest_set_lang_fortran
public :: MPII_Keyval_set_f90_proxy

! Bind to C's enum MPIR_Attr_type in mpir_attr_generic.h
enum, bind(C)
    enumerator :: MPIR_ATTR_PTR  = 0
    enumerator :: MPIR_ATTR_AINT = 1
    enumerator :: MPIR_ATTR_INT  = 3
end enum

interface

! Just need to tag the lang is Fortran, so it is fine to bind to *_lang_f77
subroutine MPIR_Grequest_set_lang_fortran(request) bind(C, name="MPII_Grequest_set_lang_f77")
    use :: mpi_c_interface_types, only : c_Request
    integer(c_Request), value, intent(in) :: request
    ! The subroutine is implemented in mpir_request.c on the C side
end subroutine MPIR_Grequest_set_lang_fortran

subroutine MPII_Keyval_set_f90_proxy(keyval) bind(C, name="MPII_Keyval_set_f90_proxy")
    use :: iso_c_binding, only : c_int
    integer(c_int), value, intent(in) :: keyval
    ! The subroutine is implemented in mpif_h/attr_proxy.c on the C side
end subroutine

end interface

contains

! Copy Fortran string to C character array, assuming the C array is one-char
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

! Copy C character array to Fortran string
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

end module mpi_c_interface_glue
