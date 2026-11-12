!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

! This file created from test/mpi/f77/coll/uallreducef.f with f77tof90
!
! Test user-defined operations.  This tests a simple commutative operation
!
      module uallreduce_uop
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
         print *, 'Invalid datatype (',datatype,') passed to user_op()'
         return
      endif

      call c_f_pointer( cin_p, cin, [count] )
      call c_f_pointer( cout_p, cout, [count] )

      do i=1, count
         cout(i) = cin(i) + cout(i)
      enddo
      end subroutine
      end module

      program main
      use mpi_f08
      use uallreduce_uop
      integer ierr, errs
      integer count, vin(65000), vout(65000), i, size
      TYPE(MPI_Op) sumop
      TYPE(MPI_Comm) comm

      errs = 0

      call mtest_init(ierr)
      call mpi_op_create( uop, .true., sumop, ierr )

      comm = MPI_COMM_WORLD
      call mpi_comm_size( comm, size, ierr )
      count = 1
      do while (count .lt. 65000)
         do i=1, count
            vin(i) = i
            vout(i) = -1
         enddo
         call mpi_allreduce( vin, vout, count, MPI_INTEGER, sumop,  &
      &                       comm, ierr )
!         Check that all results are correct
         do i=1, count
            if (vout(i) .ne. i * size) then
               errs = errs + 1
               if (errs .lt. 10) print *, "vout(",i,") = ", vout(i)
            endif
         enddo
         count = count + count
      enddo

      call mpi_op_free( sumop, ierr )

      call mtest_finalize(errs)
      end
