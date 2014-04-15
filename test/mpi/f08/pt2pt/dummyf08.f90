! This file created from test/mpi/f77/pt2pt/dummyf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2011 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!
! This file is used to disable certain compiler optimizations that
! can cause incorrect results with the test in greqf.f.  It provides a
! point where extrastate may be modified, limiting the compilers ability
! to move code around.
! The include of mpif.h is not needed in the F77 case but in the
! F90 case it is, because in that case, extrastate is defined as an
! integer (kind=MPI_ADDRESS_KIND), and the script that creates the
! F90 tests from the F77 tests looks for mpif.h
      subroutine dummyupdate( extrastate )
      use mpi_f08
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      end
