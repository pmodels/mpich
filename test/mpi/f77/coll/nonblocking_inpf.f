C -*- Mode: Fortran; -*- 
C
C (C) 2012 by Argonne National Laboratory.
C     See COPYRIGHT in top-level directory.
C
C A simple test for Fortran support of the MPI_IN_PLACE value in Alltoall[vw].
C
       program main
       implicit none
       include 'mpif.h'
       integer SIZEOFINT
       integer MAX_SIZE
       parameter (MAX_SIZE=1024)
       integer rbuf(MAX_SIZE)
       integer rdispls(MAX_SIZE), rcounts(MAX_SIZE), rtypes(MAX_SIZE)
       integer comm, rank, size, req
       integer sumval, ierr, errs
       integer iexpected, igot
       integer i, j

       errs = 0
       call mtest_init( ierr )

       comm = MPI_COMM_WORLD
       call mpi_comm_rank( comm, rank, ierr )
       call mpi_comm_size( comm, size, ierr )
       call mpi_type_size( MPI_INTEGER, SIZEOFINT, ierr )

       do i=1,MAX_SIZE
           rbuf(i) = -1
       enddo
       do i=1,size
          rbuf(i) = (i-1) * size + rank
       enddo
       call mpi_ialltoall( MPI_IN_PLACE, -1, MPI_DATATYPE_NULL,
     .                      rbuf, 1, MPI_INTEGER, comm, req, ierr )
       call mpi_wait( req, MPI_STATUS_IGNORE, ierr )
       do i=1,size
          if (rbuf(i) .ne. (rank*size + i - 1)) then
             errs = errs + 1
             print *, '[', rank, ']: IALLTOALL rbuf(', i, ') = ',
     .             rbuf(i), ', should be', rank * size + i - 1
          endif
       enddo

       do i=1,MAX_SIZE
           rbuf(i) = -1
       enddo
       do i=1,size
           rcounts(i) = i-1 + rank
           rdispls(i) = (i-1) * (2*size)
           do j=0,rcounts(i)-1
               rbuf(rdispls(i)+j+1) = 100 * rank + 10 * (i-1) + j
           enddo
       enddo
       call mpi_ialltoallv( MPI_IN_PLACE, 0, 0, MPI_DATATYPE_NULL,
     .                       rbuf, rcounts, rdispls, MPI_INTEGER,
     .                       comm, req, ierr )
       call mpi_wait( req, MPI_STATUS_IGNORE, ierr )
       do i=1,size
           do j=0,rcounts(i)-1
               iexpected = 100 * (i-1) + 10 * rank + j
               igot      = rbuf(rdispls(i)+j+1)
               if ( igot .ne. iexpected ) then
                   errs = errs + 1
                   print *, '[', rank, ']: IALLTOALLV got ', igot,
     .                   ',but expected ', iexpected,
     .                   ' for block=', i-1, ' element=', j
               endif
           enddo
       enddo

       do i=1,MAX_SIZE
           rbuf(i) = -1
       enddo
       do i=1,size
           rcounts(i) = i-1 + rank
           rdispls(i) = (i-1) * (2*size) * SIZEOFINT
           rtypes(i)  = MPI_INTEGER
           do j=0,rcounts(i)-1
               rbuf(rdispls(i)/SIZEOFINT+j+1) = 100 * rank
     .                                        + 10 * (i-1) + j
           enddo
       enddo
       call mpi_ialltoallw( MPI_IN_PLACE, 0, 0, MPI_DATATYPE_NULL,
     .                       rbuf, rcounts, rdispls, rtypes,
     .                       comm, req, ierr )
       call mpi_wait( req, MPI_STATUS_IGNORE, ierr )
       do i=1,size
           do j=0,rcounts(i)-1
               iexpected = 100 * (i-1) + 10 * rank + j
               igot      = rbuf(rdispls(i)/SIZEOFINT+j+1)
               if ( igot .ne. iexpected ) then
                   errs = errs + 1
                   print *, '[', rank, ']: IALLTOALLW got ', igot,
     .                   ',but expected ', iexpected,
     .                   ' for block=', i-1, ' element=', j
               endif
           enddo
       enddo

       do i=1,MAX_SIZE
           rbuf(i) = -1
       enddo
       do i = 1, size
           rbuf(i) = rank + (i-1)
       enddo
       call mpi_ireduce_scatter_block( MPI_IN_PLACE, rbuf, 1,
     .                                  MPI_INTEGER, MPI_SUM, comm,
     .                                  req, ierr )
       call mpi_wait( req, MPI_STATUS_IGNORE, ierr )

       sumval = size * rank + ((size-1) * size)/2
       if ( rbuf(1) .ne. sumval ) then
           errs = errs + 1
           print *, 'Ireduce_scatter_block does not get expected value.'
           print *, '[', rank, ']:', 'Got ', rbuf(1), ' but expected ',
     .              sumval, '.'
       endif

       call mtest_finalize( errs )
       call mpi_finalize( ierr )

       end
