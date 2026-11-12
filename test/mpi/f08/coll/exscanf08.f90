!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

! This file created from test/mpi/f77/coll/exscanf.f with f77tof90

      module exscan_uop
      contains
      subroutine uop( cin_p, cout_p, count, datatype )
      use mpi_f08
      use, intrinsic :: iso_c_binding, only : c_ptr, c_f_pointer
      type(c_ptr), value :: cin_p, cout_p
      integer, pointer :: cin(:), cout(:)
      integer count
      TYPE(MPI_Datatype) datatype
      integer i

      if (datatype .ne. MPI_INTEGER) then
         write(6,*) 'Invalid datatype passed to user_op()'
         return
      endif

      call c_f_pointer( cin_p, cin, [count] )
      call c_f_pointer( cout_p, cout, [count] )

      do i=1, count
         cout(i) = cin(i) + cout(i)
      enddo
      end subroutine
      end module
!
      program main
      use mpi_f08
      use exscan_uop
      integer inbuf(2), outbuf(2)
      integer ans, rank, size
      TYPE(MPI_Comm) comm
      integer errs, ierr
      TYPE(MPI_Op) sumop

      errs = 0

      call mtest_init( ierr )
!
! A simple test of exscan
      comm = MPI_COMM_WORLD

      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )

      inbuf(1) = rank
      inbuf(2) = -rank
      call mpi_exscan( inbuf, outbuf, 2, MPI_INTEGER, MPI_SUM, comm,  &
      &                 ierr )
! this process has the sum of i from 0 to rank-1, which is
! (rank)(rank-1)/2 and -i
      ans = (rank * (rank - 1))/2
      if (rank .gt. 0) then
         if (outbuf(1) .ne. ans) then
            errs = errs + 1
            print *, rank, ' Expected ', ans, ' got ', outbuf(1)
         endif
         if (outbuf(2) .ne. -ans) then
            errs = errs + 1
            print *, rank, ' Expected ', -ans, ' got ', outbuf(1)
         endif
      endif
!
! Try a user-defined operation
!
      call mpi_op_create( uop, .true., sumop, ierr )
      inbuf(1) = rank
      inbuf(2) = -rank
      call mpi_exscan( inbuf, outbuf, 2, MPI_INTEGER, sumop, comm,  &
      &                 ierr )
! this process has the sum of i from 0 to rank-1, which is
! (rank)(rank-1)/2 and -i
      ans = (rank * (rank - 1))/2
      if (rank .gt. 0) then
         if (outbuf(1) .ne. ans) then
            errs = errs + 1
            print *, rank, ' sumop: Expected ', ans, ' got ', outbuf(1)
         endif
         if (outbuf(2) .ne. -ans) then
            errs = errs + 1
            print *, rank, ' sumop: Expected ', -ans, ' got ', outbuf(1)
         endif
      endif
      call mpi_op_free( sumop, ierr )

!
! Try a user-defined operation (and don't claim it is commutative)
!
      call mpi_op_create( uop, .false., sumop, ierr )
      inbuf(1) = rank
      inbuf(2) = -rank
      call mpi_exscan( inbuf, outbuf, 2, MPI_INTEGER, sumop, comm,  &
      &                 ierr )
! this process has the sum of i from 0 to rank-1, which is
! (rank)(rank-1)/2 and -i
      ans = (rank * (rank - 1))/2
      if (rank .gt. 0) then
         if (outbuf(1) .ne. ans) then
            errs = errs + 1
            print *, rank, ' sumop2: Expected ', ans, ' got ', outbuf(1)
         endif
         if (outbuf(2) .ne. -ans) then
            errs = errs + 1
            print *, rank, ' sumop2: Expected ', -ans, ' got ',outbuf(1)
         endif
      endif
      call mpi_op_free( sumop, ierr )

      call mtest_finalize( errs )
      end
