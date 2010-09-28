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
      real    r1,r1v(2)
      double precision d1,d1v(3)
      complex c1,c1v(4)
      integer i1,i1v(5)
      character ch1,ch1v(6)
      logical l1,l1v(7)

      verbose = .false.
      errs = 0
      call mtest_init ( ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )

! Test of scalar types
      call mpi_sizeof( r1, size, ierr )
      call mpi_type_size( MPI_REAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_REAL = ", mpisize,                         &
     &            " but MPI_SIZEOF gives ", size
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
         print *, "Size of MPI_INTEGER = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( c1, size, ierr )
      call mpi_type_size( MPI_COMPLEX, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_COMPLEX = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
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
         print *, "Size of MPI_LOGICAL = ", mpisize,                        &
     &        " but MPI_SIZEOF gives ", size
      endif
!
! Test of vector types (1-dimensional)
      call mpi_sizeof( r1v, size, ierr )
      call mpi_type_size( MPI_REAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_REAL = ", mpisize,                         &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( d1v, size, ierr )
      call mpi_type_size( MPI_DOUBLE_PRECISION, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_DOUBLE_PRECISION = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( i1v, size, ierr )
      call mpi_type_size( MPI_INTEGER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_INTEGER = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( c1v, size, ierr )
      call mpi_type_size( MPI_COMPLEX, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_COMPLEX = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( ch1v, size, ierr )
      call mpi_type_size( MPI_CHARACTER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_CHARACTER = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( l1v, size, ierr )
      call mpi_type_size( MPI_LOGICAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_LOGICAL = ", mpisize,                        &
     &        " but MPI_SIZEOF gives ", size
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
