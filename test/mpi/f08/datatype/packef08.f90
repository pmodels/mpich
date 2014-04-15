! This file created from test/mpi/f77/datatype/packef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi_f08
       integer ierr, errs
       integer inbuf(10), ioutbuf(10), inbuf2(10), ioutbuf2(10)
       integer i, insize, rsize, csize, insize2
       character*(16) cbuf, coutbuf
       double precision rbuf(10), routbuf(10)
       integer packbuf(1000), pbufsize, intsize
       integer max_asizev
       parameter (max_asizev = 3)
       integer (kind=MPI_ADDRESS_KIND) aint, aintv(max_asizev)


       errs = 0
       call mtest_init( ierr )

       call mpi_type_size( MPI_INTEGER, intsize, ierr )
       pbufsize = 1000 * intsize

       call mpi_pack_external_size( 'external32', 10, MPI_INTEGER,  &
      &                              aint, ierr )
       if (aint .ne. 10 * 4) then
          errs = errs + 1
          print *, 'Expected 40 for size of 10 external32 integers', &
      &       ', got ', aint
       endif
       call mpi_pack_external_size( 'external32', 10, MPI_LOGICAL,  &
      &                              aint, ierr )
       if (aint .ne. 10 * 4) then
          errs = errs + 1
          print *, 'Expected 40 for size of 10 external32 logicals', &
      &       ', got ', aint
       endif
       call mpi_pack_external_size( 'external32', 10, MPI_CHARACTER,  &
      &                              aint, ierr )
       if (aint .ne. 10 * 1) then
          errs = errs + 1
          print *, 'Expected 10 for size of 10 external32 characters', &
      &       ', got ', aint
       endif

       call mpi_pack_external_size( 'external32', 3, MPI_INTEGER2, &
      &                              aint, ierr )
       if (aint .ne. 3 * 2) then
          errs = errs + 1
          print *, 'Expected 6 for size of 3 external32 INTEGER*2', &
      &       ', got ', aint
       endif
       call mpi_pack_external_size( 'external32', 3, MPI_INTEGER4, &
      &                              aint, ierr )
       if (aint .ne. 3 * 4) then
          errs = errs + 1
          print *, 'Expected 12 for size of 3 external32 INTEGER*4', &
      &       ', got ', aint
       endif
       call mpi_pack_external_size( 'external32', 3, MPI_REAL4, &
      &                              aint, ierr )
       if (aint .ne. 3 * 4) then
          errs = errs + 1
          print *, 'Expected 12 for size of 3 external32 REAL*4', &
      &       ', got ', aint
       endif
       call mpi_pack_external_size( 'external32', 3, MPI_REAL8, &
      &                              aint, ierr )
       if (aint .ne. 3 * 8) then
          errs = errs + 1
          print *, 'Expected 24 for size of 3 external32 REAL*8', &
      &       ', got ', aint
       endif
       if (MPI_INTEGER1 .ne. MPI_DATATYPE_NULL) then
          call mpi_pack_external_size( 'external32', 3, MPI_INTEGER1, &
      &                              aint, ierr )
          if (aint .ne. 3 * 1) then
             errs = errs + 1
             print *, 'Expected 3 for size of 3 external32 INTEGER*1', &
      &            ', got ', aint
          endif
       endif
       if (MPI_INTEGER8 .ne. MPI_DATATYPE_NULL) then
          call mpi_pack_external_size( 'external32', 3, MPI_INTEGER8, &
      &                              aint, ierr )
          if (aint .ne. 3 * 8) then
             errs = errs + 1
             print *, 'Expected 24 for size of 3 external32 INTEGER*8', &
      &            ', got ', aint
          endif
       endif

!
! Initialize values
!
       insize = 10
       do i=1, insize
          inbuf(i) = i
       enddo
       rsize = 3
       do i=1, rsize
          rbuf(i) = 1000.0 * i
       enddo
       cbuf  = 'This is a string'
       csize = 16
       insize2 = 7
       do i=1, insize2
          inbuf2(i) = 5000-i
       enddo
!
       aintv(1) = pbufsize
       aintv(2) = 0
       aintv(3) = 0
! One MPI implementation failed to increment the position; instead,
! it set the value with the amount of data packed in this call
! We use aintv(3) to detect and report this specific error
       call mpi_pack_external( 'external32', inbuf, insize, MPI_INTEGER, &
      &               packbuf, aintv(1), aintv(2), ierr )
       if (aintv(2) .le. aintv(3)) then
            print *, ' Position decreased after pack of integer!'
       endif
       aintv(3) = aintv(2)
       call mpi_pack_external( 'external32', rbuf, rsize,  &
      &               MPI_DOUBLE_PRECISION, packbuf, aintv(1),  &
      &               aintv(2), ierr )
       if (aintv(2) .le. aintv(3)) then
            print *, ' Position decreased after pack of real!'
       endif
       aintv(3) = aintv(2)
       call mpi_pack_external( 'external32', cbuf, csize,  &
      &               MPI_CHARACTER, packbuf, aintv(1),  &
      &               aintv(2), ierr )
       if (aintv(2) .le. aintv(3)) then
            print *, ' Position decreased after pack of character!'
       endif
       aintv(3) = aintv(2)
       call mpi_pack_external( 'external32', inbuf2, insize2,  &
      &               MPI_INTEGER, &
      &               packbuf, aintv(1), aintv(2), ierr )
       if (aintv(2) .le. aintv(3)) then
            print *, ' Position decreased after pack of integer (2nd)!'
       endif
       aintv(3) = aintv(2)
!
! We could try sending this with MPI_BYTE...
       aintv(2) = 0
       call mpi_unpack_external( 'external32', packbuf, aintv(1), &
      &  aintv(2), ioutbuf, insize, MPI_INTEGER, ierr )
       call mpi_unpack_external( 'external32', packbuf, aintv(1), &
      &  aintv(2), routbuf, rsize, MPI_DOUBLE_PRECISION, ierr )
       call mpi_unpack_external( 'external32', packbuf, aintv(1), &
      &  aintv(2), coutbuf, csize, MPI_CHARACTER, ierr )
       call mpi_unpack_external( 'external32', packbuf, aintv(1), &
      &  aintv(2), ioutbuf2, insize2, MPI_INTEGER, ierr )
!
! Now, test the values
!
       do i=1, insize
          if (ioutbuf(i) .ne. i) then
             errs = errs + 1
             print *, 'ioutbuf(',i,') = ', ioutbuf(i), ' expected ', i
          endif
       enddo
       do i=1, rsize
          if (routbuf(i) .ne. 1000.0 * i) then
             errs = errs + 1
             print *, 'routbuf(',i,') = ', routbuf(i), ' expected ',       &
      &                1000.0 * i
          endif
       enddo
       if (coutbuf(1:csize) .ne. 'This is a string') then
          errs = errs + 1
          print *, 'coutbuf = ', coutbuf(1:csize), ' expected ',           &
      &             'This is a string'
       endif
       do i=1, insize2
          if (ioutbuf2(i) .ne. 5000-i) then
             errs = errs + 1
             print *, 'ioutbuf2(',i,') = ', ioutbuf2(i), ' expected ',     &
      &              5000-i
          endif
       enddo
!
       call mtest_finalize( errs )
       call mpi_finalize( ierr )
       end
