!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Intercomm_merge_f08(intercomm, high, newintracomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Intercomm_merge_c

    implicit none

    type(MPI_Comm), intent(in) :: intercomm
    logical, intent(in) :: high
    type(MPI_Comm), intent(out) :: newintracomm
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: intercomm_c
    integer(c_int) :: high_c
    integer(c_Comm) :: newintracomm_c
    integer(c_int) :: ierror_c

    high_c = merge(1, 0, high)
    if (c_int == kind(0)) then
        ierror_c = MPIR_Intercomm_merge_c(intercomm%MPI_VAL, high_c, newintracomm%MPI_VAL)
    else
        intercomm_c = intercomm%MPI_VAL
        ierror_c = MPIR_Intercomm_merge_c(intercomm_c, high_c, newintracomm_c)
        newintracomm%MPI_VAL = newintracomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Intercomm_merge_f08
