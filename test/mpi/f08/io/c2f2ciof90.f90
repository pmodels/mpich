! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Test just the MPI-IO FILE object
      program main
      use mpi_f08
      integer errs, toterrs, ierr
      integer wrank
      type(MPI_Group) wgroup
      integer fsize, frank
      type(MPI_Comm) comm
      type(MPI_File) file
      type(MPI_Group) group
      integer result
      integer c2ffile

      errs = 0

      call mpi_init( ierr )

      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
      call  mpi_comm_group( MPI_COMM_WORLD, wgroup, ierr )

      call mpi_file_open( MPI_COMM_WORLD, "temp", MPI_MODE_RDWR + &
      &     MPI_MODE_DELETE_ON_CLOSE + MPI_MODE_CREATE, MPI_INFO_NULL, &
      &     file, ierr )
      if (ierr .ne. 0) then
         errs = errs + 1
      else
         errs = errs + c2ffile( file )
         call mpi_file_close( file, ierr )
      endif

      call f2cfile( file )
!     name is temp, in comm world, no info provided
      call mpi_file_get_group( file, group, ierr )
      call mpi_group_compare( group, wgroup, result, ierr )
      if (result .ne. MPI_IDENT) then
          errs = errs + 1
          print *, "Group of file not the group of comm_world"
      endif
      call mpi_group_free( group, ierr )
      call mpi_group_free( wgroup, ierr )
      call mpi_file_close( file, ierr )
!
! Summarize the errors
!
      call mpi_allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, &
      &     MPI_COMM_WORLD, ierr )
      if (wrank .eq. 0) then
         if (toterrs .eq. 0) then
            print *, ' No Errors'
         else
            print *, ' Found ', toterrs, ' errors'
         endif
      endif

      call mpi_finalize( ierr )
      end

