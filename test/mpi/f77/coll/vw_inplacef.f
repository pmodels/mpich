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
       integer ierr, errs
       integer comm, root
       integer rank, size
       integer i
       integer MAX_SIZE
       parameter (MAX_SIZE=1024)
       integer rbuf(MAX_SIZE)
       integer rdispls(MAX_SIZE), rcounts(MAX_SIZE), rtypes(MAX_SIZE)

       errs = 0
       call mtest_init( ierr )

       comm = MPI_COMM_WORLD
       call mpi_comm_rank( comm, rank, ierr )
       call mpi_comm_size( comm, size, ierr )

       do i=1,MAX_SIZE
           rbuf(i) = rank
       enddo

C Alltoallv and Alltoallw with inplace
C The test does not even check if receive buffer is processed correctly,
C because it merely aims to make sure MPI_IN_PLACE can be handled by
C Fortran MPI_Alltoall[vw]. The C version of these tests should have checked
C the buffer.
       do i=1,size
           rcounts(i) = i-1 + rank
           rdispls(i) = (i-1) * (2*size)
           rtypes(i)  = MPI_INTEGER
       enddo
       call mpi_alltoallv( MPI_IN_PLACE, 0, 0, MPI_DATATYPE_NULL,
     $                     rbuf, rcounts, rdispls, MPI_INTEGER,
     $                     comm, ierr )

       call mpi_alltoallw( MPI_IN_PLACE, 0, 0, MPI_DATATYPE_NULL,
     $                     rbuf, rcounts, rdispls, rtypes,
     $                     comm, ierr )


       call mtest_finalize( errs )
       call mpi_finalize( ierr )

       end
