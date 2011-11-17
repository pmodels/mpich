C -*- Mode: Fortran; -*- 
C
C  (C) 2011 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer ierr, errs
      integer i, ans, size, rank, color, comm, newcomm
      integer maxSize, displ
      parameter (maxSize=128)
      integer scounts(maxSize), sdispls(maxSize), stypes(maxSize)
      integer rcounts(maxSize), rdispls(maxSize), rtypes(maxSize)
      integer sbuf(maxSize), rbuf(maxSize)
      errs = 0
      
      call mtest_init( ierr )

C Get a comm
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
C      
      if (size .le. maxSize) then
C Initialize the data.  Just use this as an all to all
C Use the same test as alltoallwf.c , except displacements are in units of
C integers instead of bytes
         do i=1, size
            scounts(i) = 1
            sdispls(i) = (i-1)
            stypes(i)  = MPI_INTEGER
            sbuf(i) = rank * size + i
            rcounts(i) = 1
            rdispls(i) = (i-1)
            rtypes(i)  = MPI_INTEGER
            rbuf(i) = -1
         enddo
         call mpi_alltoallv( sbuf, scounts, sdispls, stypes,
     &        rbuf, rcounts, rdispls, rtypes, comm, ierr )     
C
C check rbuf(i) = data from the ith location of the ith send buf, or
C       rbuf(i) = (i-1) * size + i   
         do i=1, size
            ans = (i-1) * size + rank + 1
            if (rbuf(i) .ne. ans) then
               errs = errs + 1
               print *, rank, ' rbuf(', i, ') = ', rbuf(i), 
     &               ' expected ', ans
            endif
         enddo
C
C     A halo-exchange example - mostly zero counts
C
         do i=1, size
            scounts(i) = 0
            sdispls(i) = 0
            stypes(i)  = MPI_INTEGER
            sbuf(i) = -1
            rcounts(i) = 0
            rdispls(i) = 0
            rtypes(i)  = MPI_INTEGER
            rbuf(i) = -1
         enddo

         displ = 0
         if (rank .gt. 0) then
            scounts(rank-1) = 1
            rcounts(rank-1) = 1
            sdispls(rank-1)  = displ
            rdispls(rank-1)  = rank - 1
            sbuf(displ)     = rank - 1
            displ           = displ + 1
         endif
         scounts(rank)   = 1
         rcounts(rank)   = 1
         sdispls(rank)    = displ
         rdispls(rank)    = rank
         sbuf(displ)     = rank
         displ           = displ + 1
         if (rank .lt. size-1) then
            scounts(rank+1) = 1 
            rcounts(rank+1) = 1
            sdispls(rank+1)  = displ
            rdispls(rank+1)  = rank+1
            sbuf(rank+1)    = rank + 1
            displ           = displ + 1
         endif
         
         call mpi_alltoallv( sbuf, scounts, sdispls, stypes,
     &        rbuf, rcounts, rdispls, rtypes, comm, ierr )
C
C check rbuf(i) = data from the ith location of the ith send buf, or
C       rbuf(i) = (i-1) * size + i   
         if (rank .gt. 0) then
            if (rbuf(rank-1) .ne. rank-1) then
               errs = errs + 1
               print *, rank, ' rbuf(', rank-1, ') = ', rbuf(rank-1),
     &              'expected ', rank-1
            endif
         endif
         if (rbuf(rank) .ne. rank) then
            errs = errs + 1
            print *, rank, ' rbuf(', rank, ') = ', rbuf(rank),
     &           'expected ', rank
         endif
         if (rank .lt. size-1) then
            if (rbuf(rank+1) .ne. rank+1) then
               errs = errs + 1
               print *, rank, ' rbuf(', rank+1, ') = ', rbuf(rank+1),
     &              'expected ', rank+1
            endif
         endif
         do i=0,rank-2
            if (rbuf(i) .ne. -1) then
               errs = errs + 1
               print *, rank, ' rbuf(', i, ') = ', rbuf(i), 
     &              'expected -1'
            endif
         enddo
         do i=rank+2,size-1
            if (rbuf(i) .ne. -1) then
               errs = errs + 1
               print *, rank, ' rbuf(', i, ') = ', rbuf(i), 
     &              'expected -1'
            endif
         enddo
         do i=1, size
            ans = (i-1) * size + rank + 1
            if (rbuf(i) .ne. ans) then
               errs = errs + 1
               print *, rank, ' rbuf(', i, ') = ', rbuf(i), 
     &               ' expected ', ans
            endif
         enddo
      endif
      call mpi_comm_free( comm, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
      
