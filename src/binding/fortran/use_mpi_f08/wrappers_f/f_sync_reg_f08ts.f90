!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_F_sync_reg_f08ts(buf)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_F_sync_reg_cdesc

    implicit none

    type(*), dimension(..) :: buf

    integer(c_int) :: ierror_c

    ierror_c = MPIR_F_sync_reg_cdesc(buf)

end subroutine MPI_F_sync_reg_f08ts
