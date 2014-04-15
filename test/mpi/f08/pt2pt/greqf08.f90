! This file created from test/mpi/f77/pt2pt/greqf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      subroutine query_fn( extrastate, status, ierr )
      use mpi_f08
      TYPE(MPI_Status) status
      integer ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val
      logical flag

!
!    set a default status
      status%MPI_SOURCE = MPI_UNDEFINED
      status%MPI_TAG    = MPI_UNDEFINED
      flag = .false.
      call mpi_status_set_cancelled( status, flag, ierr)
      call mpi_status_set_elements( status, MPI_BYTE, 0, ierr )
      ierr = MPI_SUCCESS
      end
!
      subroutine free_fn( extrastate, ierr )
      use mpi_f08
      integer value, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      integer freefncall
      common /fnccalls/ freefncall
!
!   For testing purposes, the following print can be used to check whether
!   the free_fn is called
!      print *, 'Free_fn called'
!
      extrastate = extrastate - 1
!   The value returned by the free function is the error code
!   returned by the wait/test function
      ierr = MPI_SUCCESS
      end
!
      subroutine cancel_fn( extrastate, complete, ierr )
      use mpi_f08
      integer ierr
      logical complete
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val


      ierr = MPI_SUCCESS
      end
!
!
! This is a very simple test of generalized requests.  Normally, the
! MPI_Grequest_complete function would be called from another routine,
! often running in a separate thread.  This simple code allows us to
! check that requests can be created, tested, and waited on in the
! case where the request is complete before the wait is called.
!
! Note that MPI did *not* define a routine that can be called within
! test or wait to advance the state of a generalized request.
! Most uses of generalized requests will need to use a separate thread.
!
       program main
       use mpi_f08
       integer errs, ierr
       logical flag
       TYPE(MPI_Status) status
       TYPE(MPI_Request) request
       external query_fn, free_fn, cancel_fn
       integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

       integer freefncall
       common /fnccalls/ freefncall

       errs = 0
       freefncall = 0

       call MTest_Init( ierr )

       extrastate = 0
       call mpi_grequest_start( query_fn, free_fn, cancel_fn,  &
      &            extrastate, request, ierr )
       call mpi_test( request, flag, status, ierr )
       if (flag) then
          errs = errs + 1
          print *, 'Generalized request marked as complete'
       endif

       call mpi_grequest_complete( request, ierr )

       call MPI_Wait( request, status, ierr )

       extrastate = 1
       call mpi_grequest_start( query_fn, free_fn, cancel_fn,  &
      &                          extrastate, request, ierr )
       call mpi_grequest_complete( request, ierr )
       call mpi_wait( request, MPI_STATUS_IGNORE, ierr )
!
!      The following routine may prevent an optimizing compiler from
!      just remembering that extrastate was set in grequest_start
       call dummyupdate(extrastate)
       if (extrastate .ne. 0) then
          errs = errs + 1
          if (freefncall .eq. 0) then
              print *, 'Free routine not called'
          else
              print *, 'Free routine did not update extra_data'
              print *, 'extrastate = ', extrastate
          endif
       endif
!
       call MTest_Finalize( errs )
       call mpi_finalize( ierr )
       end
!
