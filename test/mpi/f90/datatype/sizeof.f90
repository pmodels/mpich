! -*- Mode: Fortran; -*- 
!
!  (C) 2007 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This program tests that the MPI_SIZEOF routine is implemented for the 
! predefined scalar Fortran types.  It confirms that the size of these
! types matches the size of the corresponding MPI datatypes.
!
      program main
      use mpi
      integer ierr, errs
      integer rank, size, mpisize
      logical verbose
      real    r1
      double precision d1
      complex c1
      integer i1
      character ch1
      logical l1

      verbose = .false.
      errs = 0
      call mtest_init ( ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )

!
      call mpi_sizeof( r1, size, ierr )
      call mpi_type_size( MPI_REAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_REAL = ", mpisize, " but MPI_SIZEOF gives ", &
              size
      endif

      call mpi_sizeof( d1, size, ierr )
      call mpi_type_size( MPI_DOUBLE_PRECISION, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_DOUBLE_PRECISION = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( i1, size, ierr )
      call mpi_type_size( MPI_INTEGER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_INTEGER = ", mpisize, " but MPI_SIZEOF gives ", &
              size
      endif

      call mpi_sizeof( c1, size, ierr )
      call mpi_type_size( MPI_COMPLEX, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_COMPLEX = ", mpisize, " but MPI_SIZEOF gives ", &
              size
      endif

      call mpi_sizeof( ch1, size, ierr )
      call mpi_type_size( MPI_CHARACTER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_CHARACTER = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( l1, size, ierr )
      call mpi_type_size( MPI_LOGICAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_LOGICAL = ", mpisize, " but MPI_SIZEOF gives ", &
              size
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
