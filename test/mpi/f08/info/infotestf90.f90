! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Simple info test
       program main
       use mpi_f08
       type(MPI_Info) i1, i2
       integer i, errs, ierr
       integer valuelen
       parameter (valuelen=64)
       character*(valuelen) value
       logical flag
!
       errs = 0

       call MTest_Init( ierr )

       call mpi_info_create( i1, ierr )
       call mpi_info_create( i2, ierr )

       call mpi_info_set( i1, "key1", "value1", ierr )
       call mpi_info_set( i2, "key2", "value2", ierr )

       call mpi_info_get( i1, "key2", valuelen, value, flag, ierr )
       if (flag) then
          print *, "Found key2 in info1"
          errs = errs + 1
       endif

       call MPI_Info_get( i1, "key1", 64, value, flag, ierr )
       if (.not. flag ) then
          print *, "Did not find key1 in info1"
          errs = errs + 1
       else
          if (value .ne. "value1") then
             print *, "Found wrong value (", value, "), expected value1"
             errs = errs + 1
          else
!     check for trailing blanks
             do i=7,valuelen
                if (value(i:i) .ne. " ") then
                   print *, "Found non blank in info value"
                   errs = errs + 1
                endif
             enddo
          endif
       endif

       call mpi_info_free( i1, ierr )
       call mpi_info_free( i2, ierr )

       call MTest_Finalize( errs )
       call MPI_Finalize( ierr )
       end
