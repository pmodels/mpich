C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer errs, ierr
      integer comm(4), i, rlen, ln
      integer ncomm
      character*(MPI_MAX_OBJECT_NAME) inname(4), cname
      logical MTestGetIntracomm

      errs = 0
      call mtest_init( ierr )
      
C Test the predefined communicators
      do ln=1,MPI_MAX_OBJECT_NAME
         cname(ln:ln) = 'X'
      enddo
      call mpi_comm_get_name( MPI_COMM_WORLD, cname, rlen, ierr )
      do ln=MPI_MAX_OBJECT_NAME,1,-1
         if (cname(ln:ln) .ne. ' ') then
            if (ln .ne. rlen) then
               errs = errs + 1
               print *, 'result len ', rlen,' not equal to actual len ',
     &              ln
            endif
            goto 110
         endif
      enddo
      if (cname(1:rlen) .ne. 'MPI_COMM_WORLD') then
         errs = errs + 1
         print *, 'Did not get MPI_COMM_WORLD for world'
      endif
 110  continue
C
      do ln=1,MPI_MAX_OBJECT_NAME
         cname(ln:ln) = 'X'
      enddo
      call mpi_comm_get_name( MPI_COMM_SELF, cname, rlen, ierr )
      do ln=MPI_MAX_OBJECT_NAME,1,-1
         if (cname(ln:ln) .ne. ' ') then
            if (ln .ne. rlen) then
               errs = errs + 1
               print *, 'result len ', rlen,' not equal to actual len ',
     &              ln
            endif
            goto 120
         endif
      enddo
      if (cname(1:rlen) .ne. 'MPI_COMM_SELF') then
         errs = errs + 1
         print *, 'Did not get MPI_COMM_SELF for world'
      endif
 120  continue
C
      do i = 1, 4
         if (MTestGetIntracomm( comm(i), 1, .true. )) then
            ncomm = i
            write( inname(i), '(a,i1)') 'myname',i
            call mpi_comm_set_name( comm(i), inname(i), ierr )
         else
            goto 130
         endif
      enddo
 130   continue
C
C     Now test them all
      do i=1, ncomm
         call mpi_comm_get_name( comm(i), cname, rlen, ierr )
         if (inname(i) .ne. cname) then
            errs = errs + 1
            print *, ' Expected ', inname(i), ' got ', cname
         endif
         call MTestFreeComm( comm(i) )
      enddo
C      
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
