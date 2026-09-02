!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

module mpi_c_interface_glue

use, intrinsic :: iso_c_binding, only : c_char, C_NULL_CHAR, c_ptr

implicit none

TYPE(c_ptr), BIND(C, name="MPI_F08_STATUS_IGNORE") :: MPI_F08_STATUS_IGNORE
TYPE(c_ptr), BIND(C, name="MPI_F08_STATUSES_IGNORE") :: MPI_F08_STATUSES_IGNORE
TYPE(c_ptr), BIND(C, name="MPIR_F08_MPI_BOTTOM") :: MPIR_F08_MPI_BOTTOM
TYPE(c_ptr), BIND(C, name="MPIR_F08_MPI_IN_PLACE") :: MPIR_F08_MPI_IN_PLACE

logical, save :: is_initialized = .false.
interface
    subroutine MPIX_Init_fortran() bind(c, name="MPIX_Init_fortran")
        implicit none
    end subroutine
end interface

public :: MPIR_Init_fortran
public :: MPIR_Fortran_string_f2c
public :: MPIR_Fortran_string_c2f

contains

subroutine MPIR_Init_fortran()
    USE, intrinsic :: iso_c_binding, ONLY : c_loc
    USE :: mpi_f08_link_constants, ONLY : MPI_STATUS_IGNORE, MPI_STATUSES_IGNORE, &
        MPI_BOTTOM, MPI_IN_PLACE
    implicit none

    if (is_initialized) return

    MPI_F08_STATUSES_IGNORE = c_loc(MPI_STATUSES_IGNORE)
    MPI_F08_STATUS_IGNORE = c_loc(MPI_STATUS_IGNORE)
    MPIR_F08_MPI_BOTTOM = c_loc(MPI_BOTTOM)
    MPIR_F08_MPI_IN_PLACE = c_loc(MPI_IN_PLACE)

    call MPIX_Init_fortran()
    is_initialized = .true.
end subroutine MPIR_Init_fortran

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
