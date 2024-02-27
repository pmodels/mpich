!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

program main
    use mpi_f08
    integer ierr, errs
    INTEGER :: f_status(MPI_STATUS_SIZE)
    type(mpi_status) :: f08_status

    errs = 0
    call mtest_init(ierr)

    f08_status%MPI_SOURCE = 0
    f08_status%MPI_TAG = 0
    f08_status%MPI_ERROR = 0
    call MPI_Status_f082f(f08_status, f_status)
    f_status(MPI_SOURCE) = f_status(MPI_SOURCE) + 1
    f_status(MPI_TAG) = f_status(MPI_TAG) + 10
    f_status(MPI_ERROR) = f_status(MPI_ERROR) + 100
    call MPI_Status_f2f08(f_status, f08_status)
    if (f08_status%MPI_SOURCE .ne. 1 .or. f08_status%MPI_TAG .ne. 10 .or. f08_status%MPI_ERROR .ne. 100 ) then
        errs = errs + 1
        print *, "Status f08<->f roundtrip failed. Expect fields 1, 10, 100, got ", &
            f08_status%MPI_SOURCE, f08_status%MPI_TAG, f08_status%MPI_ERROR
    endif

    f08_status%MPI_SOURCE = 0
    f08_status%MPI_TAG = 0
    f08_status%MPI_ERROR = 0
    call c_f08_status(f08_status)
    if (f08_status%MPI_SOURCE .ne. 5 .or. f08_status%MPI_TAG .ne. 50 .or. f08_status%MPI_ERROR .ne. 500 ) then
        errs = errs + 1
        print *, "Status f08<->c roundtrip failed. Expect fields 5, 50, 500, got ", &
            f08_status%MPI_SOURCE, f08_status%MPI_TAG, f08_status%MPI_ERROR
    endif

    call mtest_finalize(errs)
end program
