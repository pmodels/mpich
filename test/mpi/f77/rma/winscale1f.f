C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer ierr, errs
      integer win, intsize
      integer left, right, rank, size
      integer nrows, ncols
      parameter (nrows=25,ncols=10)
      integer buf(1:nrows,0:ncols+1)
      integer comm, group, group2, ans
      integer nneighbors, nbrs(2), i, j
      logical mtestGetIntraComm
C Include addsize defines asize as an address-sized integer
      include 'addsize.h'
      
      errs = 0
      call mtest_init( ierr )

      call mpi_type_size( MPI_INTEGER, intsize, ierr )
      do while( mtestGetIntraComm( comm, 2, .false. ) ) 
         asize = nrows * (ncols + 2) * intsize
         call mpi_win_create( buf, asize, intsize * nrows, 
     &                        MPI_INFO_NULL, comm, win, ierr )
         
C Create the group for the neighbors
         call mpi_comm_size( comm, size, ierr )
         call mpi_comm_rank( comm, rank, ierr )
         nneighbors = 0
         left = rank - 1
         if (left .lt. 0) then
            left = MPI_PROC_NULL
         else
            nneighbors = nneighbors + 1
            nbrs(nneighbors) = left
         endif
         right = rank + 1
         if (right .ge. size) then
            right = MPI_PROC_NULL
         else
            nneighbors = nneighbors + 1
            nbrs(nneighbors) = right
         endif
         call mpi_comm_group( comm, group, ierr )
         call mpi_group_incl( group, nneighbors, nbrs, group2, ierr )
         call mpi_group_free( group, ierr )
C
C Initialize the buffer 
         do i=1,nrows
            buf(i,0)       = -1
            buf(i,ncols+1) = -1
         enddo
         do j=1,ncols
            do i=1,nrows
               buf(i,j) = rank * (ncols * nrows) + i + (j-1) * nrows
            enddo
         enddo
         call mpi_win_post( group2, 0, win, ierr )
         call mpi_win_start( group2, 0, win, ierr )
C         
         asize = ncols+1
         call mpi_put( buf(1,1), nrows, MPI_INTEGER, left, asize, 
     &                 nrows, MPI_INTEGER, win, ierr )
         asize = 0
         call mpi_put( buf(1,ncols), nrows, MPI_INTEGER, right, asize, 
     &                 nrows, MPI_INTEGER, win, ierr )
C         
         call mpi_win_complete( win, ierr )
         call mpi_win_wait( win, ierr )
C
C Check the results
         if (left .ne. MPI_PROC_NULL) then
            do i=1, nrows
               ans = rank *  (ncols * nrows) - nrows + i
               if (buf(i,0) .ne. ans) then
                  errs = errs + 1
                  if (errs .le. 10) then
                     print *, ' buf(',i,'0) = ', buf(i,0)
                  endif
               endif
            enddo
         endif
         if (right .ne. MPI_PROC_NULL) then
            do i=1, nrows
               ans = (rank+1) * (ncols * nrows) +  i
               if (buf(i,ncols+1) .ne. ans) then
                  errs = errs + 1
                  if (errs .le. 10) then
                     print *, ' buf(',i,',',ncols+1,') = ',
     &                          buf(i,ncols+1)
                  endif
               endif
            enddo
         endif
         call mpi_group_free( group2, ierr )
         call mpi_win_free( win, ierr )
         call mtestFreeComm( comm )
      enddo

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
