
subroutine test11(rank)
    use mpi_f08
    implicit none

    character (len=10) :: name='test11'
    character (len=80) :: title='test 11: Isend/Irecv - 1d array - iar(10)'
    
    integer :: rank, ierr, sint, i, errors
    integer, dimension(10) :: iar
    type(MPI_Request) request
    type(MPI_Status) status
    
    if (rank .eq. 0) then
        print *, ' ===== ', trim(title), ' ====='
        do i=1,10
            iar(i)=i
        end do
        ierr = MPI_SUCCESS

        block
            ASYNCHRONOUS :: iar

            call mpi_isend(iar, 10, MPI_INTEGER, 1, 567, MPI_COMM_WORLD, request, ierr);
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,": mpi_isend exited in error (",ierr,")"
            endif

            call mpi_wait(request, status, ierr)
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,": mpi_wait exited in error (",ierr,")"
            endif

        end block

    else if (rank .eq. 1) then
        ierr = MPI_SUCCESS

        block
            ASYNCHRONOUS :: iar

            call mpi_irecv(iar, 10, MPI_INTEGER, 0, 567, MPI_COMM_WORLD, request, ierr);
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,"mpi_irecv exited in error (",ierr,")"
            endif

            call mpi_wait(request, status, ierr)
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,": mpi_wait exited in error (",ierr,")"
            endif

        end block

        errors = 0
        do i=1,10
            if (iar(i) .ne. i) then
                print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",i
                errors = errors + 1
            endif
        end do
        if (errors .eq. 0) then
            print *, "PE ", rank,": PASS - ", trim(title)
        else
            print *, "PE ", rank,": FAIL - ", trim(title)
        endif
        print *, iar(1), iar(2), iar(3), iar(4), iar(5), iar(6), iar(7), iar(8), iar(9)
    endif

    call MPI_Barrier(MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *,"PE ",rank,": ",name,": mpi_barrier exited in error (",ierr,")"
        call MPI_Abort(MPI_COMM_WORLD, 9);
    endif
end subroutine test11
