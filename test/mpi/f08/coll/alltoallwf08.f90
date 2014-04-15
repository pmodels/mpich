! This file created from test/mpi/f77/coll/alltoallwf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer ierr, errs
      integer i, intsize, ans, size, rank, color
      TYPE(MPI_Comm) comm, newcomm
      integer maxSize
      parameter (maxSize=32)
      integer scounts(maxSize), sdispls(maxSize)
      integer rcounts(maxSize), rdispls(maxSize)
      TYPE(MPI_Datatype) stypes(maxSize), rtypes(maxSize)
      integer sbuf(maxSize), rbuf(maxSize)
      errs = 0

      call mtest_init( ierr )

      call mpi_type_size( MPI_INTEGER, intsize, ierr )

! Get a comm
      call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
      call mpi_comm_size( comm, size, ierr )
      if (size .gt. maxSize) then
         call mpi_comm_rank( comm, rank, ierr )
         color = 1
         if (rank .lt. maxSize) color = 0
         call mpi_comm_split( comm, color, rank, newcomm, ierr )
         call mpi_comm_free( comm, ierr )
         comm = newcomm
         call mpi_comm_size( comm, size, ierr )
      endif
      call mpi_comm_rank( comm, rank, ierr )

      if (size .le. maxSize) then
! Initialize the data.  Just use this as an all to all
         do i=1, size
            scounts(i) = 1
            sdispls(i) = (i-1)*intsize
            stypes(i)  = MPI_INTEGER
            sbuf(i) = rank * size + i
            rcounts(i) = 1
            rdispls(i) = (i-1)*intsize
            rtypes(i)  = MPI_INTEGER
            rbuf(i) = -1
         enddo
         call mpi_alltoallw( sbuf, scounts, sdispls, stypes, &
      &        rbuf, rcounts, rdispls, rtypes, comm, ierr )
!
! check rbuf(i) = data from the ith location of the ith send buf, or
!       rbuf(i) = (i-1) * size + i
         do i=1, size
            ans = (i-1) * size + rank + 1
            if (rbuf(i) .ne. ans) then
               errs = errs + 1
               print *, rank, ' rbuf(', i, ') = ', rbuf(i),  &
      &               ' expected ', ans
            endif
         enddo
      endif
      call mpi_comm_free( comm, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end

