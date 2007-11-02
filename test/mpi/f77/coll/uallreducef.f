C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C
C Test user-defined operations.  This tests a simple commutative operation
C
      subroutine uop( cin, cout, count, datatype )
      implicit none
      integer cin(*), cout(*)
      integer count, datatype
      integer i
      
      do i=1, count
         cout(i) = cin(i) + cout(i)
      enddo
      end

      program main
      implicit none
      include 'mpif.h'
      external uop
      integer ierr, errs
      integer count, sumop, vin(65000), vout(65000), i, size
      integer comm
      
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
         call mpi_allreduce( vin, vout, count, MPI_INTEGER, sumop, 
     *                       comm, ierr )
C         Check that all results are correct
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
      call mpi_finalize(ierr)
      end
