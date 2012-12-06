C -*- Mode: Fortran; -*- 
C
C
C  (C) 2012 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      character*(MPI_MAX_OBJECT_NAME) name
      integer namelen
      integer ierr, errs

      errs = 0

      call mtest_init( ierr )
C
C Check each Fortran datatype, including the size-specific ones
C See the C version (typename.c) for the relevant MPI sections

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
