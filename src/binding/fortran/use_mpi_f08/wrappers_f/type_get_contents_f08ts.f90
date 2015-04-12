!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_get_contents_f08(datatype, max_integers, max_addresses, max_datatypes, &
    array_of_integers, array_of_addresses, array_of_datatypes, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_get_contents_c

    implicit none

    type(MPI_Datatype), intent(in) :: datatype
    integer, intent(in) :: max_integers
    integer, intent(in) :: max_addresses
    integer, intent(in) :: max_datatypes
    integer, intent(out) :: array_of_integers(max_integers)
    integer(MPI_ADDRESS_KIND), intent(out) :: array_of_addresses(max_addresses)
    type(MPI_Datatype), intent(out) :: array_of_datatypes(max_datatypes)
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    integer(c_int) :: max_integers_c
    integer(c_int) :: max_addresses_c
    integer(c_int) :: max_datatypes_c
    integer(c_int) :: array_of_integers_c(max_integers)
    integer(c_Datatype) :: array_of_datatypes_c(max_datatypes)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_get_contents_c(datatype%MPI_VAL, max_integers, max_addresses, max_datatypes, array_of_integers, &
            array_of_addresses, array_of_datatypes%MPI_VAL)
    else
        datatype_c = datatype%MPI_VAL
        max_integers_c = max_integers
        max_addresses_c = max_addresses
        max_datatypes_c = max_datatypes
        array_of_integers_c = array_of_integers
        ierror_c = MPIR_Type_get_contents_c(datatype_c, max_integers_c, max_addresses_c, max_datatypes_c, array_of_integers_c, &
            array_of_addresses, array_of_datatypes_c)
        array_of_integers = array_of_integers_c
        array_of_datatypes%MPI_VAL = array_of_datatypes_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Type_get_contents_f08
