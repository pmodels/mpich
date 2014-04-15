! This file created from test/mpi/f77/coll/redscatf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2011 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      subroutine uop( cin, cout, count, datatype )
      use mpi_f08
      integer cin(*), cout(*)
      integer count
      TYPE(MPI_Datatype) datatype
      integer i

      if (datatype .ne. MPI_INTEGER) then
         write(6,*) 'Invalid datatype ',datatype,' passed to user_op()'
         return
      endif

      do i=1, count
         cout(i) = cin(i) + cout(i)
      enddo
      end
!
! Test of reduce scatter.
!
! Each processor contributes its rank + the index to the reduction,
! then receives the ith sum
!
! Can be called with any number of processors.
!

      program main
      use mpi_f08
      integer errs, ierr, toterr
      integer maxsize
      parameter (maxsize=1024)
      integer sendbuf(maxsize), recvbuf, recvcounts(maxsize)
      integer size, rank, i, sumval
      TYPE(MPI_Comm) comm
      TYPE(MPI_Op) sumop
      external uop

      errs = 0

      call mtest_init( ierr )

      comm = MPI_COMM_WORLD

      call mpi_comm_size( comm, size, ierr )
      call mpi_comm_rank( comm, rank, ierr )

      if (size .gt. maxsize) then
      endif
      do i=1, size
         sendbuf(i) = rank + i - 1
         recvcounts(i) = 1
      enddo

      call mpi_reduce_scatter( sendbuf, recvbuf, recvcounts,  &
      &     MPI_INTEGER, MPI_SUM, comm, ierr )

      sumval = size * rank + ((size - 1) * size)/2
! recvbuf should be size * (rank + i)
      if (recvbuf .ne. sumval) then
         errs = errs + 1
         print *, "Did not get expected value for reduce scatter"
         print *, rank, " Got ", recvbuf, " expected ", sumval
      endif

      call mpi_op_create( uop, .true., sumop, ierr )
      call mpi_reduce_scatter( sendbuf, recvbuf, recvcounts,  &
      &     MPI_INTEGER, sumop, comm, ierr )

      sumval = size * rank + ((size - 1) * size)/2
! recvbuf should be size * (rank + i)
      if (recvbuf .ne. sumval) then
         errs = errs + 1
         print *, "sumop: Did not get expected value for reduce scatter"
         print *, rank, " Got ", recvbuf, " expected ", sumval
      endif
      call mpi_op_free( sumop, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
