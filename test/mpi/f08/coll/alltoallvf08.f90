! This file created from test/mpi/f77/coll/alltoallvf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2011 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer ierr, errs
      integer i, ans, size, rank, color
      TYPE(MPI_Comm) comm, newcomm
      integer maxSize, displ
      parameter (maxSize=128)
      integer scounts(maxSize), sdispls(maxSize)
      integer rcounts(maxSize), rdispls(maxSize)
      TYPE(MPI_Datatype) stype, rtype
      integer sbuf(maxSize), rbuf(maxSize)

      errs = 0

      call mtest_init( ierr )

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
!
      if (size .le. maxSize) then
! Initialize the data.  Just use this as an all to all
! Use the same test as alltoallwf.c , except displacements are in units of
! integers instead of bytes
         do i=1, size
            scounts(i) = 1
            sdispls(i) = (i-1)
            stype      = MPI_INTEGER
            sbuf(i) = rank * size + i
            rcounts(i) = 1
            rdispls(i) = (i-1)
            rtype      = MPI_INTEGER
            rbuf(i) = -1
         enddo
         call mpi_alltoallv( sbuf, scounts, sdispls, stype, &
      &        rbuf, rcounts, rdispls, rtype, comm, ierr )
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
!
!     A halo-exchange example - mostly zero counts
!
         do i=1, size
            scounts(i) = 0
            sdispls(i) = 0
            stype      = MPI_INTEGER
            sbuf(i) = -1
            rcounts(i) = 0
            rdispls(i) = 0
            rtype      = MPI_INTEGER
            rbuf(i) = -1
         enddo

!
!     Note that the arrays are 1-origin
         displ = 0
         if (rank .gt. 0) then
            scounts(1+rank-1) = 1
            rcounts(1+rank-1) = 1
            sdispls(1+rank-1) = displ
            rdispls(1+rank-1) = rank - 1
            sbuf(1+displ)     = rank
            displ             = displ + 1
         endif
         scounts(1+rank)   = 1
         rcounts(1+rank)   = 1
         sdispls(1+rank)   = displ
         rdispls(1+rank)   = rank
         sbuf(1+displ)     = rank
         displ           = displ + 1
         if (rank .lt. size-1) then
            scounts(1+rank+1) = 1
            rcounts(1+rank+1) = 1
            sdispls(1+rank+1) = displ
            rdispls(1+rank+1) = rank+1
            sbuf(1+displ)     = rank
            displ             = displ + 1
         endif

         call mpi_alltoallv( sbuf, scounts, sdispls, stype, &
      &        rbuf, rcounts, rdispls, rtype, comm, ierr )
!
!   Check the neighbor values are correctly moved
!
         if (rank .gt. 0) then
            if (rbuf(1+rank-1) .ne. rank-1) then
               errs = errs + 1
               print *, rank, ' rbuf(',1+rank-1, ') = ', rbuf(1+rank-1), &
      &              'expected ', rank-1
            endif
         endif
         if (rbuf(1+rank) .ne. rank) then
            errs = errs + 1
            print *, rank, ' rbuf(', 1+rank, ') = ', rbuf(1+rank), &
      &           'expected ', rank
         endif
         if (rank .lt. size-1) then
            if (rbuf(1+rank+1) .ne. rank+1) then
               errs = errs + 1
               print *, rank, ' rbuf(', 1+rank+1, ') = ',rbuf(1+rank+1), &
      &              'expected ', rank+1
            endif
         endif
         do i=0,rank-2
            if (rbuf(1+i) .ne. -1) then
               errs = errs + 1
               print *, rank, ' rbuf(', 1+i, ') = ', rbuf(1+i),  &
      &              'expected -1'
            endif
         enddo
         do i=rank+2,size-1
            if (rbuf(1+i) .ne. -1) then
               errs = errs + 1
               print *, rank, ' rbuf(', i, ') = ', rbuf(1+i),  &
      &              'expected -1'
            endif
         enddo
      endif
      call mpi_comm_free( comm, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end

