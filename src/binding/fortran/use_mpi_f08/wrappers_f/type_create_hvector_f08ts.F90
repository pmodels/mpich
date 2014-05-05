!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_create_hvector_f08(count, blocklength, stride, oldtype, newtype, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_create_hvector_c

    implicit none

    integer, intent(in) :: count
    integer, intent(in) :: blocklength
    integer(MPI_ADDRESS_KIND), intent(in) :: stride
    type(MPI_Datatype), intent(in) :: oldtype
    type(MPI_Datatype), intent(out) :: newtype
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_int) :: blocklength_c
    integer(MPI_ADDRESS_KIND) :: stride_c
    integer(c_Datatype) :: oldtype_c
    integer(c_Datatype) :: newtype_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_create_hvector_c(count, blocklength, stride, oldtype%MPI_VAL, newtype%MPI_VAL)
    else
        count_c = count
        blocklength_c = blocklength
        stride_c = stride
        oldtype_c = oldtype%MPI_VAL
        ierror_c = MPIR_Type_create_hvector_c(count_c, blocklength_c, stride_c, oldtype_c, newtype_c)
        newtype%MPI_VAL = newtype_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_create_hvector_f08
