! This file created from test/mpi/f77/datatype/typename3f.f with f77tof90
! -*- Mode: Fortran; -*-
!
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      character*(MPI_MAX_OBJECT_NAME) name
      integer namelen
      integer ierr, errs

      errs = 0

      call mtest_init( ierr )
!
! Check each Fortran datatype, including the size-specific ones
! See the C version (typename.c) for the relevant MPI sections

      call MPI_Type_get_name( MPI_AINT, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_AINT") then
           errs = errs + 1
           print *, "Expected MPI_AINT but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_OFFSET, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_OFFSET") then
           errs = errs + 1
           print *, "Expected MPI_OFFSET but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_COUNT, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_COUNT") then
           errs = errs + 1
           print *, "Expected MPI_COUNT but got "//name(1:namelen)
      endif

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )
      end
