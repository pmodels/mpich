!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Info_get_nkeys_f08(info, nkeys, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Info
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_Info_get_nkeys_c

    implicit none

    type(MPI_Info), intent(in) :: info
    integer, intent(out) :: nkeys
    integer, optional, intent(out) :: ierror

    integer(c_Info) :: info_c
    integer(c_int) :: nkeys_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Info_get_nkeys_c(info%MPI_VAL, nkeys)
    else
        info_c = info%MPI_VAL
        ierror_c = MPIR_Info_get_nkeys_c(info_c, nkeys_c)
        nkeys = nkeys_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Info_get_nkeys_f08
