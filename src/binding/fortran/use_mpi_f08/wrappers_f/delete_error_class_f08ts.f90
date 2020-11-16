!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

subroutine MPIX_Delete_error_class_f08(errorclass, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Delete_error_class_c

    implicit none

    integer, intent(in) :: errorclass
    integer, optional, intent(out) :: ierror

    integer(c_int) :: errorclass_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Delete_error_class_c(errorclass)
    else
        errorclass_c = errorclass
        ierror_c = MPIR_Delete_error_class_c(errorclass_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPIX_Delete_error_class_f08
