! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi_f08
       integer ierr, provided, errs, rank, size
       integer iv, isubv, qprovided
       logical flag

       errs = 0
       flag = .true.
       call mpi_finalized( flag, ierr )
       if (flag) then
          errs = errs + 1
          print *, 'Returned true for finalized before init'
       endif
       flag = .true.
       call mpi_initialized( flag, ierr )
       if (flag) then
          errs = errs + 1
          print *, 'Return true for initialized before init'
       endif

       provided = -1
       call mpi_init_thread( MPI_THREAD_MULTIPLE, provided, ierr )

       if (provided .ne. MPI_THREAD_MULTIPLE .and.  &
      &     provided .ne. MPI_THREAD_SERIALIZED .and. &
      &     provided .ne. MPI_THREAD_FUNNELED .and. &
      &     provided .ne. MPI_THREAD_SINGLE) then
          errs = errs + 1
          print *, ' Unrecognized value for provided = ', provided
       endif

       iv    = -1
       isubv = -1
       call mpi_get_version( iv, isubv, ierr )
       if (iv .ne. MPI_VERSION .or. isubv .ne. MPI_SUBVERSION) then
          errs = errs + 1
          print *, 'Version in mpif.h and get_version do not agree'
          print *, 'Version in mpif.h is ', MPI_VERSION, '.',  &
      &              MPI_SUBVERSION
          print *, 'Version in get_version is ', iv, '.', isubv
       endif
       if (iv .lt. 1 .or. iv .gt. 3) then
          errs = errs + 1
          print *, 'Version of MPI is invalid (=', iv, ')'
       endif
       if (isubv.lt.0 .or. isubv.gt.2) then
          errs = errs + 1
          print *, 'Subversion of MPI is invalid (=', isubv, ')'
       endif

       call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
       call mpi_comm_size( MPI_COMM_WORLD, size, ierr )

       flag = .false.
       call mpi_is_thread_main( flag, ierr )
       if (.not.flag) then
          errs = errs + 1
          print *, 'is_thread_main returned false for main thread'
       endif

       call mpi_query_thread( qprovided, ierr )
       if (qprovided .ne. provided) then
          errs = errs + 1
          print *,'query thread and init thread disagree on'// &
      &           ' thread level'
       endif

       call mpi_finalize( ierr )
       flag = .false.
       call mpi_finalized( flag, ierr )
       if (.not. flag) then
          errs = errs + 1
          print *, 'finalized returned false after finalize'
       endif

       if (rank .eq. 0) then
          if (errs .eq. 0) then
             print *, ' No Errors'
          else
             print *, ' Found ', errs, ' errors'
          endif
       endif

       end
