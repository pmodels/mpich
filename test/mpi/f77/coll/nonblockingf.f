C -*- Mode: Fortran; -*- 
C
C  (C) 2012 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer NUM_INTS
      parameter (NUM_INTS=2)
      integer maxSize
      parameter (maxSize=128)
      integer scounts(maxSize), sdispls(maxSize)
      integer rcounts(maxSize), rdispls(maxSize)
      integer types(maxSize)
      integer sbuf(maxSize), rbuf(maxSize)
      integer comm, size, rank, req
      integer ierr, errs
      integer ii, ans

      errs = 0

      call mtest_init(ierr)

      comm = MPI_COMM_WORLD
      call MPI_Comm_size(comm, size, ierr)
      call MPI_Comm_rank(comm, rank, ierr)
C      
      do ii = 1, size
         sbuf(2*ii-1) = ii
         sbuf(2*ii)   = ii
         sbuf(2*ii-1) = ii
         sbuf(2*ii)   = ii
         scounts(ii)  = NUM_INTS
         rcounts(ii)  = NUM_INTS
         sdispls(ii)  = (ii-1) * NUM_INTS
         rdispls(ii)  = (ii-1) * NUM_INTS
         types(ii)    = MPI_INTEGER
      enddo

      call MPI_Ibarrier(comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ibcast(sbuf, NUM_INTS, MPI_INTEGER, 0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Igather(sbuf, NUM_INTS, MPI_INTEGER,
     .                  rbuf, NUM_INTS, MPI_INTEGER,
     .                  0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Igatherv(sbuf, NUM_INTS, MPI_INTEGER,
     .                   rbuf, rcounts, rdispls, MPI_INTEGER,
     .                   0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ialltoall(sbuf, NUM_INTS, MPI_INTEGER,
     .                    rbuf, NUM_INTS, MPI_INTEGER,
     .                    comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ialltoallv(sbuf, scounts, sdispls, MPI_INTEGER,
     .                     rbuf, rcounts, rdispls, MPI_INTEGER,
     .                     comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ialltoallw(sbuf, scounts, sdispls, types,
     .                     rbuf, rcounts, rdispls, types,
     .                     comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ireduce(sbuf, rbuf, NUM_INTS, MPI_INTEGER,
     .                  MPI_SUM, 0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Iallreduce(sbuf, rbuf, NUM_INTS, MPI_INTEGER,
     .                     MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ireduce_scatter(sbuf, rbuf, rcounts, MPI_INTEGER,
     .                          MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ireduce_scatter_block(sbuf, rbuf, NUM_INTS, MPI_INTEGER,
     .                                MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Iscan(sbuf, rbuf, NUM_INTS, MPI_INTEGER,
     .                MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Iexscan(sbuf, rbuf, NUM_INTS, MPI_INTEGER,
     .                  MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )
      end
