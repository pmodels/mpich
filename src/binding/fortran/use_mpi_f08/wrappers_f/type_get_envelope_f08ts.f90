!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_get_envelope_f08(datatype, num_integers, num_addresses, num_datatypes, &
    combiner, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_get_envelope_c

    implicit none

    type(MPI_Datatype), intent(in) :: datatype
    integer, intent(out) :: num_integers
    integer, intent(out) :: num_addresses
    integer, intent(out) :: num_datatypes
    integer, intent(out) :: combiner
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    integer(c_int) :: num_integers_c
    integer(c_int) :: num_addresses_c
    integer(c_int) :: num_datatypes_c
    integer(c_int) :: combiner_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_get_envelope_c(datatype%MPI_VAL, num_integers, num_addresses, num_datatypes, &
            combiner)
    else
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Type_get_envelope_c(datatype_c, num_integers_c, num_addresses_c, num_datatypes_c, &
            combiner_c)
        num_integers = num_integers_c
        num_addresses = num_addresses_c
        num_datatypes = num_datatypes_c
        combiner = combiner_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_get_envelope_f08
