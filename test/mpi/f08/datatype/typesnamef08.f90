! This file created from test/mpi/f77/datatype/typesnamef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi_f08
       character*(MPI_MAX_OBJECT_NAME) cname
       integer rlen, ln
       integer errs, ierr
       TYPE(MPI_Datatype) ntype1, ntype2

       errs = 0

       call MTest_Init( ierr )

       call mpi_type_vector( 10, 1, 100, MPI_INTEGER, ntype1, ierr )
       rlen = -1
       cname = 'XXXXXX'
       call mpi_type_get_name( ntype1, cname, rlen, ierr )
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, ' Expected length 0, got ', rlen
       endif
       rlen = 0
       do ln=MPI_MAX_OBJECT_NAME,1,-1
          if (cname(ln:ln) .ne. ' ') then
             rlen = ln
             goto 100
          endif
       enddo
 100   continue
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, 'Datatype name is not all blank'
       endif
!
! now add a name, then dup
       call mpi_type_set_name( ntype1, 'a vector type', ierr )
       call mpi_type_dup( ntype1, ntype2, ierr )
       rlen = -1
       cname = 'XXXXXX'
       call mpi_type_get_name( ntype2, cname, rlen, ierr )
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, ' (type2) Expected length 0, got ', rlen
       endif
       rlen = 0
       do ln=MPI_MAX_OBJECT_NAME,1,-1
          if (cname(ln:ln) .ne. ' ') then
             rlen = ln
             goto 110
          endif
       enddo
 110   continue
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, ' (type2) Datatype name is not all blank'
       endif

       call mpi_type_free( ntype1, ierr )
       call mpi_type_free( ntype2, ierr )

       call MTest_Finalize( errs )
       call MPI_Finalize( ierr )

       end
