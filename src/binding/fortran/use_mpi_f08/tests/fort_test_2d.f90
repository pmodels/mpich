
!
program main

use mpi_f08

implicit none

integer ierr
integer size, rank
integer sint, iar(10), iar_2d(9,9), iar_3d(9,9,9)
type(MPI_Status) status
integer i, j, k
integer junk(1000)
integer save_my_rank
integer errors

call mpi_init(ierr)

call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

print *, 'Hello, I am ', rank, ' of ', size, '.'


save_my_rank = rank

! ----------------------------------------------------------
if (rank .eq. 0) then
    sint = 789
    write (0,*) ' ===== Trivial send/recv ====='
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
    if (sint .eq. 789) then
        print *, "PE ", rank,": PASS - Trivial send/recv"
    else
        print *, "PE ", rank,": FAIL - Trivial send/recv"
    endif
endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

! ----------------------------------------------------------
if (rank .eq. 0) then
    write (0,*) ' ===== Send/Recv iar(2:8:2) ====='
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
    errors = 0
    do i=2,8,2
        if (iar(i) .ne. i) then
            print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",i
            errors = errors + 1
        endif
    end do
    if (errors .eq. 0) then
        print *, "PE ", rank,": PASS - Send/Recv iar(2:8:2)"
    else
        print *, "PE ", rank,": FAIL - Send/Recv iar(2:8:2)"
    endif
endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

! ----------------------------------------------------------
if (rank .eq. 0) then
    write (0, *) " ======== Simple 2d array test (iar_2d) ======== "
    print *, " ======== Simple 2d array test (iar_2d) ======== "

    do i=1,9
        do j=1,9
            iar_2d(j,i) = (j * 10) + i
        end do
    end do

    do j=1,9
        print *, iar_2d(1,j), iar_2d(2,j), iar_2d(3,j), iar_2d(4,j), iar_2d(5,j), iar_2d(6,j), iar_2d(7,j), iar_2d(8,j), iar_2d(9,j)
    end do

else
    do i=1,9
        do j=1,9
            iar_2d(j,i) = 0
        end do
    end do

endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

if (rank .eq. 0) then
    print *, 'PE 0: Sending: loc(iar_2d)=',loc(iar_2d)
    ierr = MPI_SUCCESS
    call mpi_send(iar_2d, 81, MPI_INTEGER, 1, 789, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, ": mpi_send exited in error (",ierr,")"
    endif
else
    print *, 'PE 1: Receiving: loc(iar_2d)=',loc(iar_2d)
    ierr = MPI_SUCCESS
    call mpi_recv(iar_2d, 81, MPI_INTEGER, 0, 789, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, "(1): mpi_recv exited in error (",ierr,")"
    endif
    errors = 0
    do i=1,9
        do j=1,9
            if (iar_2d(j,i) .ne. (j * 10) + i) then
                print *, "rank 1: iar_2d(", j, ",", i, ")=", iar_2d(j,i), ", expected ", (j * 10) + i
                errors = errors + 1
            endif
        end do
    end do
    if (errors .eq. 0) then
        print *, "PE ", rank,": PASS - Send/Recv iar_2d()"
    else
        print *, "PE ", rank,": FAIL - Send/Recv iar_2d()"
    endif
endif

! ----------------------------------------------------------
if (rank .eq. 0) then
    write (0, *) " ======== Simple 3d array test (iar_3d) ======== "
    print *, " ======== Simple 3d array test (iar_3d) ======== "

    do i=1,9
        do j=1,9
            do k=1,9
                iar_3d(k,j,i) = (k * 100) + (j * 10) + i
            end do
        end do
    end do

else
    do i=1,9
        do j=1,9
            do k=1,9
                iar_3d(k, j,i) = 0
            end do
        end do
    end do

endif

call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

if (rank .eq. 0) then
    print *, 'PE 0: Sending: loc(iar_3d)=',loc(iar_3d)
    ierr = MPI_SUCCESS
    call mpi_send(iar_3d, 729, MPI_INTEGER, 1, 891, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, ": mpi_send exited in error (",ierr,")"
    endif
else
    print *, 'PE 1: Receiving: loc(iar_3d)=',loc(iar_3d)
    ierr = MPI_SUCCESS
    call mpi_recv(iar_3d, 729, MPI_INTEGER, 0, 891, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *, "PE ", rank, "(1): mpi_recv exited in error (",ierr,")"
    endif

    errors = 0

    do i=1,9
        do j=1,9
            do k=1,9
                if (iar_3d(k, j,i) .ne. (k * 100) + (j * 10) + i) then
                    print *, "rank 1: iar_3d(", k, ",", j, ",", i, ")=", iar_3d(k, j,i), ", expected ", (k * 100) + (j * 10) + i
                    errors = errors + 1
                endif
            end do
        end do
    end do

    if (errors .eq. 0) then
        print *, "PE ", rank,": PASS - Send/Recv iar_3d()"
    else
        print *, "PE ", rank,": FAIL - Send/Recv iar_3d()"
    endif
endif

! ----------------------------------------------------------
write (0, *) "PE ", rank, ": calling last barrier"
print *, "PE ", rank, ": calling last barrier"
call MPI_Barrier(MPI_COMM_WORLD, ierr);
if (ierr .ne. MPI_SUCCESS) then
    print *, "PE ", rank, ": mpi_barrier exited in error (",ierr,")"
    call MPI_Abort(MPI_COMM_WORLD, 9);
endif

write (0, *) "PE ", rank, ": calling Finalize"
print *, "PE ", rank, ": calling Finalize"
call mpi_finalize(ierr)
end
