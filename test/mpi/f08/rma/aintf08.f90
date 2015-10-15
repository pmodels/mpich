! -*- Mode: Fortran; -*-
!
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!

! This program tests MPI_Aint_add/diff in MPI-3.1.
! The two functions are often used in RMA code.
! See https://svn.mpi-forum.org/trac/mpi-forum-web/ticket/349

program main
    use mpi_f08
    integer :: rank, nproc
    integer :: ierr, errs
    integer :: array(0:1023)
    integer :: val, target_rank;
    integer(kind=MPI_ADDRESS_KIND) :: bases(0:1), disp, offset
    integer(kind=MPI_ADDRESS_KIND) :: winsize
    type(MPI_WIN) :: win
    integer :: intsize

    errs = 0
    call mtest_init(ierr);
    call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, nproc, ierr)

    if (rank == 0 .and. nproc /= 2) then
      print *, 'Must run with 2 ranks'
      call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
    endif

! Get the base address in the middle of the array
    if (rank == 0) then
      target_rank = 1
      array(0) = 1234
      call MPI_Get_address(array(512), bases(0), ierr)
    else if (rank == 1) then
      target_rank = 0
      array(1023) = 1234
      call MPI_Get_address(array(512), bases(1), ierr)
    endif

! Exchange bases
    call MPI_Type_size(MPI_INTEGER, intsize, ierr);
    call MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bases, 1, MPI_AINT, MPI_COMM_WORLD, ierr)
    call MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, win, ierr)

    winsize = intsize*1024
    call MPI_Win_attach(win, array, winsize, ierr)

! Do MPI_Aint addressing arithmetic
    if (rank == 0) then
        disp = intsize*511
        offset = MPI_Aint_add(bases(1), disp) ! offset points to array(1023)
    else if (rank == 1) then
        disp = intsize*512
        offset = MPI_Aint_diff(bases(0), disp) ! offset points to array(0)
    endif

! Get value and verify it
    call MPI_Win_fence(MPI_MODE_NOPRECEDE, win, ierr)
    call MPI_Get(val, 1, MPI_INTEGER, target_rank, offset, 1, MPI_INTEGER, win, ierr)
    call MPI_Win_fence(MPI_MODE_NOSUCCEED, win, ierr)

    if (val /= 1234) then
      errs = errs + 1
      print *, rank, ' -- Got', val, 'expected 1234'
    endif

    call MPI_Win_detach(win, array, ierr)
    call MPI_Win_free(win, ierr)

    call MTest_Finalize(errs)
    call MPI_Finalize(ierr);
end
