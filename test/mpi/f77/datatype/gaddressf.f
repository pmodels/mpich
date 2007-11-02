C -*- Mode: Fortran; -*- 
C
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer max_asizev
      parameter (max_asizev=2)
      include 'typeaints.h'
      integer iarray(200), gap, intsize
      integer ierr, errs

      errs = 0

      call MPI_Init(ierr)

      call MPI_Get_address( iarray(1), aintv(1), ierr )
      call MPI_Get_address( iarray(200), aintv(2), ierr )
      gap = aintv(2) - aintv(1)

      call MPI_Type_size( MPI_INTEGER, intsize, ierr )

      if (gap .ne. 199 * intsize) then
         errs = errs + 1
         print *, ' Using get_address, computed a gap of ', gap
         print *, ' Expected a gap of ', 199 * intsize
      endif
      if (errs .gt. 0) then
          print *, ' Found ', errs, ' errors'
      else
          print *, ' No Errors'
      endif

      call MPI_Finalize( ierr )
      end
