! This file created from test/mpi/f77/coll/inplacef.f with f77tof90
! -*- Mode: Fortran; -*-
!
! (C) 2005 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
! This is a simple test that Fortran support the MPI_IN_PLACE value
!
       program main
       use mpi_f08
       integer ierr, errs
       integer root
       TYPE(MPI_Comm) comm
       integer rank, size
       integer i
       integer MAX_SIZE
       parameter (MAX_SIZE=1024)
       integer rbuf(MAX_SIZE), rdispls(MAX_SIZE), rcount(MAX_SIZE), &
      &      sbuf(MAX_SIZE)

       errs = 0
       call mtest_init( ierr )

       comm = MPI_COMM_WORLD
       call mpi_comm_rank( comm, rank, ierr )
       call mpi_comm_size( comm, size, ierr )

       root = 0
! Gather with inplace
       do i=1,size
          rbuf(i) = - i
       enddo
       rbuf(1+root) = root
       if (rank .eq. root) then
          call mpi_gather( MPI_IN_PLACE, 1, MPI_INTEGER, rbuf, 1, &
      &         MPI_INTEGER, root, comm, ierr )
          do i=1,size
             if (rbuf(i) .ne. i-1) then
                errs = errs + 1
                print *, '[',rank,'] rbuf(', i, ') = ', rbuf(i),  &
      &                   ' in gather'
             endif
          enddo
       else
          call mpi_gather( rank, 1, MPI_INTEGER, rbuf, 1, MPI_INTEGER, &
      &         root, comm, ierr )
       endif

! Gatherv with inplace
       do i=1,size
          rbuf(i) = - i
          rcount(i) = 1
          rdispls(i) = i-1
       enddo
       rbuf(1+root) = root
       if (rank .eq. root) then
          call mpi_gatherv( MPI_IN_PLACE, 1, MPI_INTEGER, rbuf, rcount, &
      &         rdispls, MPI_INTEGER, root, comm, ierr )
          do i=1,size
             if (rbuf(i) .ne. i-1) then
                errs = errs + 1
                print *, '[', rank, '] rbuf(', i, ') = ', rbuf(i),  &
      &                ' in gatherv'
             endif
          enddo
       else
          call mpi_gatherv( rank, 1, MPI_INTEGER, rbuf, rcount, rdispls, &
      &         MPI_INTEGER, root, comm, ierr )
       endif

! Scatter with inplace
       do i=1,size
          sbuf(i) = i
       enddo
       rbuf(1) = -1
       if (rank .eq. root) then
          call mpi_scatter( sbuf, 1, MPI_INTEGER, MPI_IN_PLACE, 1, &
      &         MPI_INTEGER, root, comm, ierr )
       else
          call mpi_scatter( sbuf, 1, MPI_INTEGER, rbuf, 1, &
      &         MPI_INTEGER, root, comm, ierr )
          if (rbuf(1) .ne. rank+1) then
             errs = errs + 1
             print *, '[', rank, '] rbuf  = ', rbuf(1), &
      &            ' in scatter'
          endif
       endif

       call mtest_finalize( errs )

       end
