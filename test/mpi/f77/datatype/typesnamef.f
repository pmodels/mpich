C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
       program main
       implicit none
       include 'mpif.h'
       character*(MPI_MAX_OBJECT_NAME) cname
       integer rlen, ln
       integer ntype1, ntype2, errs, ierr

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
C
C now add a name, then dup
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
