C -*- Mode: Fortran; -*-
C
C  (C) 2015 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C
C  This test tests absolute datatypes and MPI_BOTTOM in mixed
C  Fortran and C code.  MPI requires MPI_Get_address return
C  the same value in all languages.
C  See discussion on p.652 of MPI-3.0

      program main
      implicit none
      include 'mpif.h'
      integer :: R(5)
      integer :: type, ierr, aoblen(1), aotype(1)
      integer (kind=mpi_address_kind) :: aodisp(1)
      integer errs

      errs = 0
      R = (/1, 2, 3, 4, 5/)

      call mtest_init(ierr)

      ! create an absolute datatype for array r
      aoblen(1) = 5
      call MPI_Get_address(R, aodisp(1), ierr)
      aotype(1) = MPI_INTEGER
      call MPI_Type_create_struct(1,aoblen,aodisp,aotype, type, ierr)
      call c_routine(type, errs)

      call MPI_Type_free(type, ierr);
      call mtest_finalize(errs)
      call MPI_Finalize(ierr)
      end
