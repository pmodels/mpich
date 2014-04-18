!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Error_class_f08(errorcode, errorclass, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Error_class_c

    implicit none

    integer, intent(in) :: errorcode
    integer, intent(out) :: errorclass
    integer, optional, intent(out) :: ierror

    integer(c_int) :: errorcode_c
    integer(c_int) :: errorclass_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Error_class_c(errorcode, errorclass)
    else
        errorcode_c = errorcode
        ierror_c = MPIR_Error_class_c(errorcode_c, errorclass_c)
        errorclass = errorclass_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Error_class_f08
