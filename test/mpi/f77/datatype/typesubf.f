C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer errs, ierr
      integer maxn, maxm
      parameter (maxn=10,maxm=15)
      integer fullsizes(2), subsizes(2), starts(2)
      integer fullarr(maxn,maxm),subarr(maxn-3,maxm-4)
      integer i,j, ssize
      integer newtype, size, rank, ans

      errs = 0
      call mtest_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
C
C Create a Fortran-style subarray
      fullsizes(1) = maxn
      fullsizes(2) = maxm
      subsizes(1)  = maxn - 3
      subsizes(2)  = maxm - 4
C starts are from zero, even in Fortran
      starts(1)    = 1
      starts(2)    = 2
C In Fortran 90 notation, the original array is
C    integer a(maxn,maxm)
C and the subarray is
C    a(1+1:(maxn-3) +(1+1)-1,2+1:(maxm-4)+(2+1)-1)
C i.e., a (start:(len + start - 1),...)
      call mpi_type_create_subarray( 2, fullsizes, subsizes, starts, 
     &         MPI_ORDER_FORTRAN, MPI_INTEGER, newtype, ierr )
      call mpi_type_commit( newtype, ierr )
C
C Prefill the array
      do j=1, maxm
         do i=1, maxn
            fullarr(i,j) = (i-1) + (j-1) * maxn
         enddo
      enddo
      do j=1, subsizes(2)
         do i=1, subsizes(1)
            subarr(i,j) = -1
         enddo
      enddo
      ssize = subsizes(1)*subsizes(2)
      call mpi_sendrecv( fullarr, 1, newtype, rank, 0, 
     &                   subarr, ssize, MPI_INTEGER, rank, 0, 
     &                   MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr )
C
C Check the data
      do j=1, subsizes(2)
         do i=1, subsizes(1)
            ans = (i+starts(1)-1) + (j+starts(2)-1) * maxn
            if (subarr(i,j) .ne. ans) then
               errs = errs + 1
               if (errs .le. 10) then
                  print *, rank, 'subarr(',i,',',j,') = ', subarr(i,j)
               endif
            endif
         enddo
      enddo

      call mpi_type_free( newtype, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
