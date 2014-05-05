!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_create_f90_complex_f08(p, r, newtype, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_create_f90_complex_c

    implicit none

    integer, intent(in) :: p
    integer, intent(in) :: r
    type(MPI_Datatype), intent(out) :: newtype
    integer, optional, intent(out) :: ierror

    integer(c_int) :: p_c
    integer(c_int) :: r_c
    integer(c_Datatype) :: newtype_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_create_f90_complex_c(p, r, newtype%MPI_VAL)
    else
        p_c = p
        r_c = r
        ierror_c = MPIR_Type_create_f90_complex_c(p_c, r_c, newtype_c)
        newtype%MPI_VAL = newtype_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_create_f90_complex_f08
