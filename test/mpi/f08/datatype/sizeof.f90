!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

! This program tests that the storage_size routine is implemented for the
! predefined scalar Fortran types.  It confirms that the size of these
! types matches the size of the corresponding MPI datatypes.
!
      program main
      use mpi_f08
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
      size = storage_size(r1) / 8
      call mpi_type_size( MPI_REAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_REAL = ", mpisize,                         &
     &            " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(d1) / 8
      call mpi_type_size( MPI_DOUBLE_PRECISION, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_DOUBLE_PRECISION = ", mpisize, &
              " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(i1) / 8
      call mpi_type_size( MPI_INTEGER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_INTEGER = ", mpisize,                      &
     &            " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(c1) / 8
      call mpi_type_size( MPI_COMPLEX, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_COMPLEX = ", mpisize,                      &
     &            " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(ch1) / 8
      call mpi_type_size( MPI_CHARACTER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_CHARACTER = ", mpisize, &
              " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(l1) / 8
      call mpi_type_size( MPI_LOGICAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_LOGICAL = ", mpisize,                        &
     &        " but STORAGE_SIZE gives ", size
      endif
!
! Test of vector types (1-dimensional)
      size = storage_size(r1v) / 8
      call mpi_type_size( MPI_REAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_REAL = ", mpisize,                         &
     &            " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(d1v) / 8
      call mpi_type_size( MPI_DOUBLE_PRECISION, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_DOUBLE_PRECISION = ", mpisize, &
              " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(i1v) / 8
      call mpi_type_size( MPI_INTEGER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_INTEGER = ", mpisize,                      &
     &            " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(c1v) / 8
      call mpi_type_size( MPI_COMPLEX, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_COMPLEX = ", mpisize,                      &
     &            " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(ch1v) / 8
      call mpi_type_size( MPI_CHARACTER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_CHARACTER = ", mpisize, &
              " but STORAGE_SIZE gives ", size
      endif

      size = storage_size(l1v) / 8
      call mpi_type_size( MPI_LOGICAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_LOGICAL = ", mpisize,                        &
     &        " but STORAGE_SIZE gives ", size
      endif

      call mtest_finalize( errs )

      end
