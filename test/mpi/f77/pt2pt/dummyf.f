C -*- Mode: Fortran; -*- 
C
C  (C) 2011 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C
C This file is used to disable certain compiler optimizations that
C can cause incorrect results with the test in greqf.f.  It provides a 
C point where extrastate may be modified, limiting the compilers ability
C to move code around.
C The include of mpif.h is not needed in the F77 case but in the 
C F90 case it is, because in that case, extrastate is defined as an
C integer (kind=MPI_ADDRESS_KIND), and the script that creates the
C F90 tests from the F77 tests looks for mpif.h
      subroutine dummyupdate( extrastate )
      include 'mpif.h'
      include 'attr1aints.h'
      end
