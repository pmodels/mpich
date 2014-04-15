! This file created from test/mpi/f77/coll/reducelocalf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2009 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!
! Test Fortran MPI_Reduce_local with MPI_OP_SUM and with user-defined operation.
!
      subroutine user_op( invec, outvec, count, datatype )
      use mpi_f08
      integer invec(*), outvec(*)
      integer count
      TYPE(MPI_Datatype) datatype
      integer ii

      if (datatype .ne. MPI_INTEGER) then
         write(6,*) 'Invalid datatype passed to user_op()'
         return
      endif

      do ii=1, count
         outvec(ii) = invec(ii) * 2 + outvec(ii)
      enddo

      end

      program main
      use mpi_f08
      integer max_buf_size
      parameter (max_buf_size=65000)
      integer vin(max_buf_size), vout(max_buf_size)
      external user_op
      integer ierr, errs
      integer count
      TYPE(MPI_Op) myop
      integer ii

      errs = 0

      call mtest_init(ierr)

      count = 0
      do while (count .le. max_buf_size )
         do ii = 1,count
            vin(ii) = ii
            vout(ii) = ii
         enddo
         call mpi_reduce_local( vin, vout, count, &
      &                          MPI_INTEGER, MPI_SUM, ierr )
!        Check if the result is correct
         do ii = 1,count
            if ( vin(ii) .ne. ii ) then
               errs = errs + 1
            endif
            if ( vout(ii) .ne. 2*ii ) then
               errs = errs + 1
            endif
         enddo
         if ( count .gt. 0 ) then
            count = count + count
         else
            count = 1
         endif
      enddo

      call mpi_op_create( user_op, .false., myop, ierr )

      count = 0
      do while (count .le. max_buf_size)
         do ii = 1, count
            vin(ii) = ii
            vout(ii) = ii
         enddo
         call mpi_reduce_local( vin, vout, count, &
      &                          MPI_INTEGER, myop, ierr )
!        Check if the result is correct
         do ii = 1, count
            if ( vin(ii) .ne. ii ) then
               errs = errs + 1
            endif
            if ( vout(ii) .ne. 3*ii ) then
               errs = errs + 1
            endif
         enddo
         if ( count .gt. 0 ) then
            count = count + count
         else
            count = 1
         endif
      enddo

      call mpi_op_free( myop, ierr )

      call mtest_finalize(errs)
      call mpi_finalize(ierr)

      end
