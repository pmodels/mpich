C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C   
C Test just MPI-RMA
C
      program main
      implicit none
      include 'mpif.h'
      integer errs, toterrs, ierr
      integer wrank, wsize
      integer wgroup, info, req, win
      integer result
      integer c2fwin
C The integer asize must be of ADDRESS_KIND size
      include 'addsize.h'
      errs = 0

      call mpi_init( ierr )

C
C Test passing a Fortran MPI object to C
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
      asize = 0
      call mpi_win_create( 0, asize, 1, MPI_INFO_NULL, 
     $     MPI_COMM_WORLD, win, ierr )
      errs = errs + c2fwin( win )
      call mpi_win_free( win, ierr )

C
C Test using a C routine to provide the Fortran handle
      call f2cwin( win )
C     no info, in comm world, created with no memory (base address 0,
C     displacement unit 1
      call mpi_win_free( win, ierr )
      
C
C Summarize the errors
C
      call mpi_allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM,
     $     MPI_COMM_WORLD, ierr )
      if (wrank .eq. 0) then
         if (toterrs .eq. 0) then
            print *, ' No Errors'
         else
            print *, ' Found ', toterrs, ' errors'
         endif
      endif

      call mpi_finalize( ierr )
      stop
      end
      
