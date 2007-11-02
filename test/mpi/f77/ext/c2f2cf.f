C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer errs, toterrs, ierr
      integer wrank, wsize
      integer wgroup, info, req
      integer fsize, frank
      integer comm, group, type, op, errh, result
      integer c2fcomm, c2fgroup, c2ftype, c2finfo, c2frequest,
     $     c2ferrhandler, c2fop
      character value*100
      logical   flag
      errs = 0

      call mpi_init( ierr )

C
C Test passing a Fortran MPI object to C
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
      errs = errs + c2fcomm( MPI_COMM_WORLD )
      call mpi_comm_group( MPI_COMM_WORLD, wgroup, ierr )
      errs = errs + c2fgroup( wgroup )
      call mpi_group_free( wgroup, ierr )

      call mpi_info_create( info, ierr )
      call mpi_info_set( info, "host", "myname", ierr )
      call mpi_info_set( info, "wdir", "/rdir/foo", ierr )
      errs = errs + c2finfo( info )
      call mpi_info_free( info, ierr )

      errs = errs + c2ftype( MPI_INTEGER )

      call mpi_irecv( 0, 0, MPI_INTEGER, MPI_ANY_SOURCE, MPI_ANY_TAG,
     $     MPI_COMM_WORLD, req, ierr )
      call mpi_cancel( req, ierr )
      errs = errs + c2frequest( req )
      call mpi_wait( req, MPI_STATUS_IGNORE, ierr )

      errs = errs + c2ferrhandler( MPI_ERRORS_RETURN )

      errs = errs + c2fop( MPI_SUM )

C
C Test using a C routine to provide the Fortran handle
      call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )

      call f2ccomm( comm )
      call mpi_comm_size( comm, fsize, ierr )
      call mpi_comm_rank( comm, frank, ierr )
      if (fsize.ne.wsize .or. frank.ne.wrank) then
         errs = errs + 1
         print *, "Comm(fortran) has wrong size or rank"
      endif
      
      call f2cgroup( group )
      call mpi_group_size( group, fsize, ierr )
      call mpi_group_rank( group, frank, ierr )
      if (fsize.ne.wsize .or. frank.ne.wrank) then
         errs = errs + 1
         print *, "Group(fortran) has wrong size or rank"
      endif
      call mpi_group_free( group, ierr )

      call f2ctype( type )
      if (type .ne. MPI_INTEGER) then
         errs = errs + 1
         print *, "Datatype(fortran) is not MPI_INT"
      endif
      
      call f2cinfo( info )
      call mpi_info_get( info, "host", 100, value, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Info test for host returned false"
      else if (value .ne. "myname") then
         errs = errs + 1
         print *, "Info test for host returned ", value
      endif
      call mpi_info_get( info, "wdir", 100, value, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Info test for wdir returned false"
      else if (value .ne. "/rdir/foo") then
         errs = errs + 1
         print *, "Info test for wdir returned ", value
      endif
      call mpi_info_free( info, ierr )

      call f2cop( op )
      if (op .ne. MPI_SUM) then
          errs = errs + 1
          print *, "Fortran MPI_SUM not MPI_SUM in C"
      endif

      call f2cerrhandler( errh )
      if (errh .ne. MPI_ERRORS_RETURN) then
          errs = errs + 1
          print *,"Fortran MPI_ERRORS_RETURN not MPI_ERRORS_RETURN in C"
      endif
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
      
