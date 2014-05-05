!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Type_commit_f08(datatype, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype
    use :: mpi_c_interface, only : c_Datatype
    use :: mpi_c_interface, only : MPIR_Type_commit_c

    implicit none

    type(MPI_Datatype), intent(inout) :: datatype
    integer, optional, intent(out) :: ierror

    integer(c_Datatype) :: datatype_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Type_commit_c(datatype%MPI_VAL)
    else
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_Type_commit_c(datatype_c)
        datatype%MPI_VAL = datatype_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Type_commit_f08
