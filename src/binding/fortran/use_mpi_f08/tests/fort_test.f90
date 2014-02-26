!
program main

use mpi_f08

implicit none

integer ierr
integer size, rank
integer sint, iar(10)
type(MPI_Status) status
integer i
integer junk(100)
integer save_my_rank

call mpi_init(ierr)

call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

print *, 'Hello, I am ', rank, ' of ', size, '.'

save_my_rank = rank

! ----------------------------------------------------------
if (rank .eq. 0) then
    sint = 789
    print *, ' ===== Trivial send/recv ====='
    print *, "rank 0 sends ", sint
    ierr = MPI_SUCCESS
    call mpi_send(sint, 1, MPI_INTEGER, 1, 567, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, ": mpi_send exited in error (",ierr,")"
    endif
else
    ierr = MPI_SUCCESS
    call mpi_recv(sint, 1, MPI_INTEGER, 0, 567, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank,": mpi_recv exited in error (",ierr,")"
    endif
    print *, "rank 1 receives ",sint
endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

! ----------------------------------------------------------
if (rank .eq. 0) then
    print *, ' ===== Send/Recv iar(10) ====='
    do i=1,10
        iar(i)=i
    end do
    ierr = MPI_SUCCESS
    print *, 'PE 0: loc(iar)=',loc(iar)
    call mpi_send(iar, 10, MPI_INTEGER, 1, 567, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, ": mpi_send exited in error (",ierr,")"
    endif
else if (rank .eq. 1) then
    ierr = MPI_SUCCESS
    print *, 'PE 1: loc(iar)=',loc(iar)
    call mpi_recv(iar, 10, MPI_INTEGER, 0, 567, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE 1: mpi_recv exited in error (",ierr,")"
    endif
    do i=1,10
        if (iar(i) .ne. i) then
            print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",i
        endif
    end do
endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

! ----------------------------------------------------------
if (rank .eq. 0) then
    print *, ' ===== Send/Recv iar(2:7) ====='
    do i=1,10
        iar(i)=i
    end do
    ierr = MPI_SUCCESS
    print *, 'PE ', rank, '(0),[', save_my_rank, ']: loc(iar)=',loc(iar)
    print *, 'PE 0: loc(iar)=',loc(iar)
    call mpi_send(iar(2:7), 10, MPI_INTEGER, 1, 678, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, "(0): mpi_send exited in error (",ierr,")"
    endif
else if (rank .eq. 1) then
    ierr = MPI_SUCCESS
    print *, 'PE 1: loc(iar)=',loc(iar)
    call mpi_recv(iar(2:7), 10, MPI_INTEGER, 0, 678, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, "(1): mpi_recv exited in error (",ierr,")"
    endif
    do i=2,7
        if (iar(i) .ne. i) then
            print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",i
        endif
    end do
endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

! ----------------------------------------------------------
if (rank .eq. 0) then
    print *, ' ===== Send/Recv iar(2:8:2) ====='
    do i=1,10
        iar(i)=i
    end do
    ierr = MPI_SUCCESS
    print *, 'PE ', rank, '(0),[', save_my_rank, ']: loc(iar)=',loc(iar)
    print *, 'PE 0: loc(iar)=',loc(iar)
    call mpi_send(iar(2:8:2), 10, MPI_INTEGER, 1, 678, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, "(0): mpi_send exited in error (",ierr,")"
    endif
else if (rank .eq. 1) then
    ierr = MPI_SUCCESS
    print *, 'PE 1: loc(iar)=',loc(iar)
    call mpi_recv(iar(2:8:2), 10, MPI_INTEGER, 0, 678, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, "(1): mpi_recv exited in error (",ierr,")"
    endif
    do i=2,8,2
        if (iar(i) .ne. i) then
            print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",i
        endif
    end do
endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif
! ----------------------------------------------------------

call mpi_finalize(ierr)
end
