C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C Test just the MPI-IO FILE object
      program main
      implicit none
      include 'mpif.h'
      integer errs, toterrs, ierr
      integer wrank
      integer wgroup
      integer fsize, frank
      integer comm, file, group, result
      integer c2ffile

      errs = 0

      call mpi_init( ierr )

      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
      call  mpi_comm_group( MPI_COMM_WORLD, wgroup, ierr )

      call mpi_file_open( MPI_COMM_WORLD, "temp", MPI_MODE_RDWR +
     $     MPI_MODE_DELETE_ON_CLOSE + MPI_MODE_CREATE, MPI_INFO_NULL,
     $     file, ierr ) 
      if (ierr .ne. 0) then
         errs = errs + 1
      else
         errs = errs + c2ffile( file )
         call mpi_file_close( file, ierr )
      endif

      call f2cfile( file )
C     name is temp, in comm world, no info provided
      call mpi_file_get_group( file, group, ierr )
      call mpi_group_compare( group, wgroup, result, ierr )
      if (result .ne. MPI_IDENT) then
          errs = errs + 1
          print *, "Group of file not the group of comm_world"
      endif
      call mpi_group_free( group, ierr )
      call mpi_group_free( wgroup, ierr )
      call mpi_file_close( file, ierr )
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
      
