! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!     This program is Fortran version of dgraph_unwgt.c
!     Specify a distributed graph of a bidirectional ring of the MPI_COMM_WORLD,
!     i.e. everyone only talks to left and right neighbors.

      logical function validate_dgraph(dgraph_comm)
      use mpi_f08

      type(MPI_Comm) dgraph_comm
      integer     comm_topo
      integer     src_sz, dest_sz
      integer     ierr;
      logical     wgt_flag;
      integer     srcs(2), dests(2)

      integer     world_rank, world_size;
      integer     idx, nbr_sep

      comm_topo = MPI_UNDEFINED
      call MPI_Topo_test(dgraph_comm, comm_topo, ierr);
      if (comm_topo .ne. MPI_DIST_GRAPH) then
          validate_dgraph = .false.
          write(6,*) "dgraph_comm is NOT of type MPI_DIST_GRAPH."
          return
      endif

      call MPI_Dist_graph_neighbors_count(dgraph_comm, &
      &                                    src_sz, dest_sz, wgt_flag, &
      &                                    ierr)
      if (ierr .ne. MPI_SUCCESS) then
          validate_dgraph = .false.
          write(6,*) "MPI_Dist_graph_neighbors_count() fails!"
          return
      endif
      if (wgt_flag) then
          validate_dgraph = .false.
          write(6,*) "dgraph_comm is NOT created with MPI_UNWEIGHTED."
          return
      endif

      if (src_sz .ne. 2 .or. dest_sz .ne. 2) then
          validate_dgraph = .false.
          write(6,*) "source or destination edge array is not size 2."
          write(6,"('src_sz = ',I3,', dest_sz = ',I3)") src_sz, dest_sz
          return
      endif

      call MPI_Dist_graph_neighbors(dgraph_comm, &
      &                              src_sz, srcs, MPI_UNWEIGHTED, &
      &                              dest_sz, dests, MPI_UNWEIGHTED, &
      &                              ierr)
      if (ierr .ne. MPI_SUCCESS) then
          validate_dgraph = .false.
          write(6,*) "MPI_Dist_graph_neighbors() fails!"
          return
      endif

!     Check if the neighbors returned from MPI are really
!     the nearest neighbors that within a ring.
      call MPI_Comm_rank(MPI_COMM_WORLD, world_rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, world_size, ierr)
 
      do idx = 1, src_sz
          nbr_sep = iabs(srcs(idx) - world_rank)
          if (nbr_sep .ne. 1 .and. nbr_sep .ne. (world_size-1)) then
              validate_dgraph = .false.
              write(6,"('srcs[',I3,']=',I3, &
      &                  ' is NOT a neighbor of my rank',I3)") &
      &              idx, srcs(idx), world_rank
              return
          endif
      enddo
      do idx = 1, dest_sz
          nbr_sep = iabs(dests(idx) - world_rank)
          if (nbr_sep .ne. 1 .and. nbr_sep .ne. (world_size-1)) then
              validate_dgraph = .false.
              write(6,"('dests[',I3,']=',I3, &
      &                  ' is NOT a neighbor of my rank',I3)") &
      &              idx, dests(idx), world_rank
              return
          endif
      enddo

      validate_dgraph = .true.
      return
      end

      integer function ring_rank(world_size, in_rank)
      integer world_size, in_rank
      if (in_rank .ge. 0 .and. in_rank .lt. world_size) then
          ring_rank = in_rank
          return
      endif
      if (in_rank .lt. 0 ) then
          ring_rank = in_rank + world_size
          return
      endif
      if (in_rank .ge. world_size) then
          ring_rank = in_rank - world_size
          return
      endif
      ring_rank = -99999
      return
      end



      program dgraph_unwgt
      use mpi_f08

      integer    ring_rank
      external   ring_rank
      logical    validate_dgraph
      external   validate_dgraph
      integer    errs, ierr

      type(MPI_Comm) dgraph_comm
      integer    world_size, world_rank
      integer    src_sz, dest_sz
      integer    degs(1)
      integer    srcs(2), dests(2)

      errs = 0
      call MTEST_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, world_rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, world_size, ierr)

      srcs(1) = world_rank
      degs(1) = 2;
      dests(1) = ring_rank(world_size, world_rank-1)
      dests(2) = ring_rank(world_size, world_rank+1)
      call MPI_Dist_graph_create(MPI_COMM_WORLD, 1, srcs, degs, dests, &
      &                           MPI_UNWEIGHTED, MPI_INFO_NULL, &
      &                          .true., dgraph_comm, ierr)
      if (ierr .ne. MPI_SUCCESS) then
          write(6,*) "MPI_Dist_graph_create() fails!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      if (.not. validate_dgraph(dgraph_comm)) then
          write(6,*) "MPI_Dist_graph_create() does not create" &
      &               //"a bidirectional ring graph!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      call MPI_Comm_free(dgraph_comm, ierr)

! now create one with MPI_WEIGHTS_EMPTY
! NOTE that MPI_WEIGHTS_EMPTY was added in MPI-3 and does not
! appear before then.  Incluing this test means that this test cannot
! be compiled if the MPI version is less than 3 (see the testlist file)

      degs(1) = 0;
      call MPI_Dist_graph_create(MPI_COMM_WORLD, 1, srcs, degs, dests, &
      &                           MPI_WEIGHTS_EMPTY, MPI_INFO_NULL, &
      &                          .true., dgraph_comm, ierr)
      if (ierr .ne. MPI_SUCCESS) then
          write(6,*) "MPI_Dist_graph_create() fails!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      call MPI_Comm_free(dgraph_comm, ierr)

      src_sz   = 2
      srcs(1)  = ring_rank(world_size, world_rank-1)
      srcs(2)  = ring_rank(world_size, world_rank+1)
      dest_sz  = 2
      dests(1) = ring_rank(world_size, world_rank-1)
      dests(2) = ring_rank(world_size, world_rank+1)
      call MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, &
      &                                    src_sz, srcs, &
      &                                    MPI_UNWEIGHTED, &
      &                                    dest_sz, dests, &
      &                                    MPI_UNWEIGHTED, &
      &                                    MPI_INFO_NULL, .true., &
      &                                    dgraph_comm, ierr)
      if (ierr .ne. MPI_SUCCESS) then
          write(6,*) "MPI_Dist_graph_create_adjacent() fails!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      if (.not. validate_dgraph(dgraph_comm)) then
          write(6,*) "MPI_Dist_graph_create_adjacent() does not create" &
      &               //"a bidirectional ring graph!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      call MPI_Comm_free(dgraph_comm, ierr)

! now create one with MPI_WEIGHTS_EMPTY
      src_sz   = 0
      dest_sz  = 0
      call MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD, &
      &                                    src_sz, srcs, &
      &                                    MPI_WEIGHTS_EMPTY, &
      &                                    dest_sz, dests, &
      &                                    MPI_WEIGHTS_EMPTY, &
      &                                    MPI_INFO_NULL, .true., &
      &                                    dgraph_comm, ierr)
      if (ierr .ne. MPI_SUCCESS) then
          write(6,*) "MPI_Dist_graph_create_adjacent() fails!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      call MPI_Comm_free(dgraph_comm, ierr)

      call MTEST_Finalize(errs)
      call MPI_Finalize(ierr)
      end
