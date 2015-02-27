! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!  See COPYRIGHT in top-level directory.
!
program main
use mpi_f08

implicit none
!include 'mpif.h'

! Fortran 2008 equivalent of src/mpi/romio/test/coll_test.c

integer FILESIZE
parameter (FILESIZE=32*32*32*4)

! A 32^3 array. For other array sizes, change FILESIZE above and
! array_of_gsizes below.

! Uses collective I/O. Writes a 3D block-distributed array to a file
! corresponding to the global array in row-major (C) order, reads it
! back, and checks that the data read is correct.

! Note that the file access pattern is noncontiguous.

integer i, ndims, array_of_gsizes(3)
integer order, intsize, nprocs, j, array_of_distribs(3)
integer array_of_dargs(3), array_of_psizes(3)
integer readbuf(FILESIZE), writebuf(FILESIZE), bufcount
integer mynod, tmpbuf(FILESIZE), array_size, argc
integer ierr
character*256 str   ! used to store the filename
integer errs, toterrs
integer(MPI_OFFSET_KIND) :: disp

type(MPI_Datatype) ::  newtype
type(MPI_Status) :: status
type(MPI_Request) :: request
type(MPI_File) :: fh

errs = 0
str = "iotest.txt"

call MPI_INIT(ierr)
call MPI_COMM_SIZE(MPI_COMM_WORLD, nprocs, ierr)
call MPI_COMM_RANK(MPI_COMM_WORLD, mynod, ierr)

! create the distributed array filetype

ndims = 3
order = MPI_ORDER_FORTRAN

array_of_gsizes(1) = 32
array_of_gsizes(2) = 32
array_of_gsizes(3) = 32

array_of_distribs(1) = MPI_DISTRIBUTE_BLOCK
array_of_distribs(2) = MPI_DISTRIBUTE_BLOCK
array_of_distribs(3) = MPI_DISTRIBUTE_BLOCK

array_of_dargs(1) = MPI_DISTRIBUTE_DFLT_DARG
array_of_dargs(2) = MPI_DISTRIBUTE_DFLT_DARG
array_of_dargs(3) = MPI_DISTRIBUTE_DFLT_DARG

do i=1, ndims
   array_of_psizes(i) = 0
end do

call MPI_DIMS_CREATE(nprocs, ndims, array_of_psizes, ierr)

call MPI_TYPE_CREATE_DARRAY(nprocs, mynod, ndims,   &
     array_of_gsizes, array_of_distribs, array_of_dargs,  &
     array_of_psizes, order, MPI_INTEGER, newtype, ierr)

call MPI_TYPE_COMMIT(newtype, ierr)

! initialize writebuf

call MPI_TYPE_SIZE(newtype, bufcount, ierr)
call MPI_TYPE_SIZE(MPI_INTEGER, intsize, ierr)
bufcount = bufcount/intsize
do i=1, bufcount
   writebuf(i) = 1
end do

do i=1, FILESIZE
   tmpbuf(i) = 0
end do

call MPI_IRECV(tmpbuf, 1, newtype, mynod, 10, MPI_COMM_WORLD, request, ierr)
call MPI_SEND(writebuf, bufcount, MPI_INTEGER, mynod, 10, MPI_COMM_WORLD, ierr)
call MPI_WAIT(request, status, ierr)

j = 1
array_size = array_of_gsizes(1) * array_of_gsizes(2) * array_of_gsizes(3)
do i=1, array_size
   if (tmpbuf(i) .ne. 0) then
      writebuf(j) = i
      j = j + 1
   end if
end do

! end of initialization

! write the array to the file

call MPI_FILE_OPEN(MPI_COMM_WORLD, str, MPI_MODE_CREATE+MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr)

disp = 0
call MPI_FILE_SET_VIEW(fh, disp, MPI_INTEGER, newtype, "native", MPI_INFO_NULL, ierr)
call MPI_FILE_IWRITE_ALL(fh, writebuf, bufcount, MPI_INTEGER, request, ierr)
call MPI_WAIT(request, status, ierr)
call MPI_FILE_CLOSE(fh, ierr)

!now read it back

call MPI_FILE_OPEN(MPI_COMM_WORLD, str, MPI_MODE_CREATE+MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr)

call MPI_FILE_SET_VIEW(fh, disp, MPI_INTEGER, newtype, "native", MPI_INFO_NULL, ierr)
call MPI_FILE_IREAD_ALL(fh, readbuf, bufcount, MPI_INTEGER, request, ierr)
call MPI_WAIT(request, status, ierr)
call MPI_FILE_CLOSE(fh, ierr)

! check the data read
do i=1, bufcount
   if (readbuf(i) .ne. writebuf(i)) then
      errs = errs + 1
       print *, 'Node ', mynod, '  readbuf ', readbuf(i),  &
          '  writebuf ', writebuf(i), '  i', i
   end if
end do

call MPI_TYPE_FREE(newtype, ierr)

if (mynod .eq. 0) then
    call MPI_FILE_DELETE(str, MPI_INFO_NULL, ierr)
    if (ierr .ne. MPI_SUCCESS) then
        errs = errs + 1
    endif
endif

call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, MPI_COMM_WORLD, ierr )

if (mynod .eq. 0) then
  if( toterrs .gt. 0 ) then
     print *, 'Found ', toterrs, ' errors'
  else
     print *, ' No Errors'
  endif
endif

call MPI_FINALIZE(ierr)

stop
end
