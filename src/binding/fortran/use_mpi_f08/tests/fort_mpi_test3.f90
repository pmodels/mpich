
subroutine test3(rank)
    use mpi_f08
    implicit none

    character (len=10) :: name='test3'
    character (len=80) :: title='test 3: Send/Recv array slice - iar(2:7)'
    
    integer :: rank, ierr, i, errors
    integer, dimension(10) ::  iar, iar_check
    type(MPI_Status) status
    
    if (rank .eq. 0) then
        print *, ' ===== ', trim(title), ' ====='
        do i=1,10
            iar(i)=i
        end do
        ierr = MPI_SUCCESS
        call mpi_send(iar(2:7), 6, MPI_INTEGER, 1, 678, MPI_COMM_WORLD, ierr);
        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,": mpi_send exited in error (",ierr,")"
        endif
    else if (rank .eq. 1) then
        do i=1,10
            iar(i)=0
            iar_check(i)=0
        end do
        do i=2,7
            iar_check(i)=i
        end do
        ierr = MPI_SUCCESS
        call mpi_recv(iar(2:7), 6, MPI_INTEGER, 0, 678, MPI_COMM_WORLD, status, ierr);
        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,"mpi_recv exited in error (",ierr,")"
        endif
        errors = 0
        do i=1,10
            if (iar(i) .ne. iar_check(i)) then
                print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",iar_check(i)
                errors = errors + 1
            endif
        end do
        if (errors .eq. 0) then
            print *, "PE ", rank,": PASS - test 3 - ", trim(title)
        else
            print *, "PE ", rank,": FAIL - test 3 - ", trim(title)
        endif
        print *, iar(1), iar(2), iar(3), iar(4), iar(5), iar(6), iar(7), iar(8), iar(9)
    endif

    call MPI_Barrier(MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *,"PE ",rank,": ",name,": mpi_barrier exited in error (",ierr,")"
        call MPI_Abort(MPI_COMM_WORLD, 9);
    endif
end subroutine test3
