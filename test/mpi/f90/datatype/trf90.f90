! -*- Mode: Fortran; -*- 
!
!  (C) 2011 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Based on a isample program that triggered a segfault in MPICH2
program testf90_mpi
  use mpi
  implicit none
  integer :: rk_mpi, ierr, ctype

  call MPI_Init(ierr)
  call MPI_Type_create_f90_real(15, MPI_UNDEFINED, rk_mpi, ierr)
  call MPI_Type_contiguous(19, rk_mpi, ctype, ierr)
  call MPI_Type_commit(ctype, ierr)
  call MPI_Finalize(ierr)

end program testf90_mpi
