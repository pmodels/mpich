      program main
      include 'mpif.h'
      integer ierr
      integer errs, wrank
      integer f2ctype
C
      call mtest_init( ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
C
      errs = 0
C
      errs = errs + f2ctype( MPI_CHAR, 0 )
      errs = errs + f2ctype( MPI_SIGNED_CHAR, 1 )
      errs = errs + f2ctype( MPI_UNSIGNED_CHAR, 2 )
      errs = errs + f2ctype( MPI_WCHAR, 3 )
      errs = errs + f2ctype( MPI_SHORT, 4 )
      errs = errs + f2ctype( MPI_UNSIGNED_SHORT, 5 )
      errs = errs + f2ctype( MPI_INT, 6 )
      errs = errs + f2ctype( MPI_UNSIGNED, 7 )
      errs = errs + f2ctype( MPI_LONG, 8 )
      errs = errs + f2ctype( MPI_UNSIGNED_LONG, 9 )
      errs = errs + f2ctype( MPI_FLOAT, 10 )
      errs = errs + f2ctype( MPI_DOUBLE, 11 )
      errs = errs + f2ctype( MPI_FLOAT_INT, 12 )
      errs = errs + f2ctype( MPI_DOUBLE_INT, 13 )
      errs = errs + f2ctype( MPI_LONG_INT, 14 )
      errs = errs + f2ctype( MPI_SHORT_INT, 15 )
      errs = errs + f2ctype( MPI_2INT, 16 )
      if (MPI_LONG_DOUBLE .ne. MPI_TYPE_NULL) then
          errs = errs + f2ctype( MPI_LONG_DOUBLE, 17 )
          errs = errs + f2ctype( MPI_LONG_DOUBLE_INT, 21 )
      endif
      if (MPI_LONG_LONG .ne. MPI_TYPE_NULL) then
          errs = errs + f2ctype( MPI_LONG_LONG_INT, 18 )
          errs = errs + f2ctype( MPI_LONG_LONG, 19 )
          errs = errs + f2ctype( MPI_UNSIGNED_LONG_LONG, 20 )
      endif
C
C Summarize the errors
C
      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end 
