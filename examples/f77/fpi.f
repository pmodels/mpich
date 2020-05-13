C
C Copyright (C) by Argonne National Laboratory
C     See COPYRIGHT in top-level directory
C

C****************************************************************************
C   pi.f - compute pi by integrating f(x) = 4/(1 + x**2)
C   Each node:
C    1) receives the number of rectangles used in the approximation.
C    2) calculates the areas of it's rectangles.
C    3) Synchronizes for a global summation.
C   Node 0 prints the result.
C
C  Variables:
C
C    pi  the calculated result
C    n   number of points of integration.
C    x           midpoint of each rectangle's interval
C    f           function to integrate
C    sum,pi      area of rectangles
C    tmp         temporary scratch space for global summation
C    i           do loop index
C****************************************************************************
      program main

      include 'mpif.h'

      double precision  PI25DT
      parameter        (PI25DT = 3.141592653589793238462643d0)

      double precision  mypi, pi, h, sum, x, f, a
      integer n, myid, numprocs, i, rc
C                                 function to integrate
      f(a) = 4.d0 / (1.d0 + a*a)

      call MPI_INIT( ierr )
      call MPI_COMM_RANK( MPI_COMM_WORLD, myid, ierr )
      call MPI_COMM_SIZE( MPI_COMM_WORLD, numprocs, ierr )
      print *, "Process ", myid, " of ", numprocs, " is alive"

      sizetype   = 1
      sumtype    = 2
      
 10   if ( myid .eq. 0 ) then
         write(6,98)
 98      format('Enter the number of intervals: (0 quits)')
         read(5,99) n
 99      format(i10)
      endif
      
      call MPI_BCAST(n,1,MPI_INTEGER,0,MPI_COMM_WORLD,ierr)

C                                 check for quit signal
      if ( n .le. 0 ) goto 30

C                                 calculate the interval size
      h = 1.0d0/n

      sum  = 0.0d0
      do 20 i = myid+1, n, numprocs
         x = h * (dble(i) - 0.5d0)
         sum = sum + f(x)
 20   continue
      mypi = h * sum

C                                 collect all the partial sums
      call MPI_REDUCE(mypi,pi,1,MPI_DOUBLE_PRECISION,MPI_SUM,0,
     $     MPI_COMM_WORLD,ierr)

C                                 node 0 prints the answer.
      if (myid .eq. 0) then
         write(6, 97) pi, abs(pi - PI25DT)
 97      format('  pi is approximately: ', F18.16,
     +          '  Error is: ', F18.16)
      endif

      goto 10

 30   call MPI_FINALIZE(rc)
      stop
      end
