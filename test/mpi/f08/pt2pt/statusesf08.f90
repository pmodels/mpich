! This file created from test/mpi/f77/pt2pt/statusesf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
!     Test support for MPI_STATUS_IGNORE and MPI_STATUSES_IGNORE
      use mpi_f08
      integer nreqs
      parameter (nreqs = 100)
      TYPE(MPI_Request) reqs(nreqs)
      integer ierr, rank, i
      integer errs

      ierr = -1
      errs = 0
      call mpi_init( ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         print *, 'Unexpected return from MPI_INIT', ierr
      endif

      ierr = -1
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         print *, 'Unexpected return from MPI_COMM_WORLD', ierr
      endif
      do i=1, nreqs, 2
         ierr = -1
         call mpi_isend( MPI_BOTTOM, 0, MPI_BYTE, rank, i, &
      &        MPI_COMM_WORLD, reqs(i), ierr )
         if (ierr .ne. MPI_SUCCESS) then
            errs = errs + 1
            print *, 'Unexpected return from MPI_ISEND', ierr
         endif
         ierr = -1
         call mpi_irecv( MPI_BOTTOM, 0, MPI_BYTE, rank, i, &
      &        MPI_COMM_WORLD, reqs(i+1), ierr )
         if (ierr .ne. MPI_SUCCESS) then
            errs = errs + 1
            print *, 'Unexpected return from MPI_IRECV', ierr
         endif
      enddo

      ierr = -1
      call mpi_waitall( nreqs, reqs, MPI_STATUSES_IGNORE, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         print *, 'Unexpected return from MPI_WAITALL', ierr
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
