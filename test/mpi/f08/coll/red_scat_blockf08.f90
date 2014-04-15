! This file created from test/mpi/f77/coll/red_scat_blockf.f with f77tof90
! -*- Mode: Fortran; -*-
!
! (C) 2012 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
! A simple test for Fortran support of Reduce_scatter_block
! with or withoutMPI_IN_PLACE.
!
       program main
       use mpi_f08
       integer MAX_SIZE
       parameter (MAX_SIZE=1024)
       integer sbuf(MAX_SIZE), rbuf(MAX_SIZE)
       integer rank, size
       TYPE(MPI_Comm) comm
       integer sumval, ierr, errs, i

       errs = 0
       call mtest_init( ierr )

       comm = MPI_COMM_WORLD
       call mpi_comm_rank( comm, rank, ierr )
       call mpi_comm_size( comm, size, ierr )

       do i = 1, size
           sbuf(i) = rank + (i-1)
       enddo

       call MPI_Reduce_scatter_block(sbuf, rbuf, 1, MPI_INTEGER, &
      &                               MPI_SUM, comm, ierr)

       sumval = size * rank + ((size-1) * size)/2
       if ( rbuf(1) .ne. sumval ) then
           errs = errs + 1
           print *, 'Reduce_scatter_block does not get expected value.'
           print *, '[', rank, ']', 'Got ', rbuf(1), ' but expected ', &
      &              sumval, '.'
       endif

! Try MPI_IN_PLACE
       do i = 1, size
           rbuf(i) = rank + (i-1)
       enddo
       call MPI_Reduce_scatter_block(MPI_IN_PLACE, rbuf, 1, MPI_INTEGER, &
      &                               MPI_SUM, comm, ierr)
       if ( rbuf(1) .ne. sumval ) then
           errs = errs + 1
           print *, 'Reduce_scatter_block does not get expected value.'
           print *, '[', rank, ']', 'Got ', rbuf(1), ' but expected ', &
      &              sumval, '.'
       endif

       call mtest_finalize( errs )
       call mpi_finalize( ierr )

       end
