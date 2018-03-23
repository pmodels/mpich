! -*- Mode: Fortran; -*- 
!
!  (C) 2011 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Based on a sample program that triggered a segfault in MPICH
program testf90_mpi
  use mpi
  implicit none
  integer errs
  integer :: rk_mpi, ierr, ctype

  errs = 0
  call mtest_init(ierr)

  call MPI_Type_create_f90_real(15, MPI_UNDEFINED, rk_mpi, ierr)
  call MPI_Type_contiguous(19, rk_mpi, ctype, ierr)
  call MPI_Type_commit(ctype, ierr)
  call MPI_Type_free(ctype, ierr)

  call mtest_finalize(errs)

end program testf90_mpi
