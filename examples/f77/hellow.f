C
C Copyright 2026 Argonne National Laboratory
C SPDX-License-Identifier: Apache-2.0
C

      program main

      include 'mpif.h'

      integer ierr, myid, numprocs

      call MPI_INIT( ierr )
      call MPI_COMM_RANK( MPI_COMM_WORLD, myid, ierr )
      call MPI_COMM_SIZE( MPI_COMM_WORLD, numprocs, ierr )
      print *, "Process ", myid, " of ", numprocs, " is alive"

      call MPI_FINALIZE(rc)
      stop
      end
