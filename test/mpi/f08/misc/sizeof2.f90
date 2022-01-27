!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

        program main
          use mpi_f08
          integer ierr, errs
          integer size1, size2
          real    a
          real    d(20)
          double precision b
          complex c

          errs = 0
          call mtest_init(ierr)
          size1 = storage_size(errs) / 8
          call mpi_type_size( MPI_INTEGER, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "integer size is ", size2, " storage_size claims ", size1
          endif

          size1 = storage_size(a) / 8
          call mpi_type_size( MPI_REAL, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "real size is ", size2, " storage_size claims ", size1
          endif

          size1 = storage_size(b) / 8
          call mpi_type_size( MPI_DOUBLE_PRECISION, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "double precision size is ", size2, " storage_size claims ", size1
          endif

          size1 = storage_size(c) / 8
          call mpi_type_size( MPI_COMPLEX, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "complex size is ", size2, " storage_size claims ", size1
          endif

          size1 = storage_size(d) / 8
          call mpi_type_size( MPI_REAL, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "real array size is ", size2, " storage_size claims ", size1
          endif

          call mtest_finalize( errs )

        end program main
