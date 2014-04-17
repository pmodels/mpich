! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This test contributed by Kim McMahon, Cray
!
      program main
      use mpi
      implicit none
      integer ierr, i, j, type, count,errs
      parameter (count = 4)
      integer rank, size, xfersize
      integer status(MPI_STATUS_SIZE)
      integer blocklens(count), displs(count)
      double precision,dimension(:,:),allocatable :: sndbuf, rcvbuf
      logical verbose

      verbose = .false. 
      call mtest_init ( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
      if (size .lt. 2) then
         print *, "Must have at least 2 processes"
         call MPI_Abort( MPI_COMM_WORLD, 1, ierr )
      endif

      errs = 0
      allocate(sndbuf(7,100))
      allocate(rcvbuf(7,100))

      do j=1,100
        do i=1,7
           sndbuf(i,j) = (i+j) * 1.0
         enddo
      enddo

      do i=1,count
         blocklens(i) = 7
      enddo

! bug occurs when first two displacements are 0
      displs(1) = 0 
      displs(2) = 0 
      displs(3) = 10
      displs(4) = 10 

      call mpi_type_indexed( count, blocklens, displs*blocklens(1),  &
      &                         MPI_DOUBLE_PRECISION, type, ierr )

      call mpi_type_commit( type, ierr )

! send using this new type

      if (rank .eq. 0) then

          call mpi_send( sndbuf(1,1), 1, type, 1, 0, MPI_COMM_WORLD,ierr )

      else if (rank .eq. 1) then
       
          xfersize=count * blocklens(1)
          call mpi_recv( rcvbuf(1,1), xfersize, MPI_DOUBLE_PRECISION, 0, 0, &
           &   MPI_COMM_WORLD,status, ierr )


! Values that should be sent

        if (verbose) then
!       displacement = 0
            j=1
            do i=1, 7
               print*,'sndbuf(',i,j,') = ',sndbuf(i,j)
            enddo

!       displacement = 10
            j=11
            do i=1,7
               print*,'sndbuf(',i,j,') = ',sndbuf(i,j)
            enddo
            print*,' '

! Values received
            do j=1,count
                do i=1,7
                    print*,'rcvbuf(',i,j,') = ',rcvbuf(i,j)
                enddo
            enddo
        endif

! Error checking
        do j=1,2
           do i=1,7
             if (rcvbuf(i,j) .ne. sndbuf(i,1)) then
                print*,'ERROR in rcvbuf(',i,j,')'
                print*,'Received ', rcvbuf(i,j),' expected ',sndbuf(i,11)
                errs = errs+1
             endif
           enddo
        enddo

        do j=3,4
           do i=1,7
              if (rcvbuf(i,j) .ne. sndbuf(i,11)) then
                print*,'ERROR in rcvbuf(',i,j,')'
                print*,'Received ', rcvbuf(i,j),' expected ',sndbuf(i,11)
                errs = errs+1
              endif
           enddo
        enddo
      endif
!
      call mpi_type_free( type, ierr )
      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
