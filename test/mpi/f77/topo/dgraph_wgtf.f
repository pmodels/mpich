!     This program is Fortran version of dgraph_unwgt.c
!     Specify a distributed graph of a bidirectional ring of the MPI_COMM_WORLD,
!     i.e. everyone only talks to left and right neighbors.

      logical function validate_dgraph(dgraph_comm)
      implicit none
      include 'mpif.h'

      integer     dgraph_comm
      integer     comm_topo
      integer     src_sz, dest_sz
      integer     ierr;
      logical     wgt_flag;
      integer     srcs(2), dests(2)
      integer     src_wgts(2), dest_wgts(2)

      integer     world_rank, world_size;
      integer     idx, nbr_sep

      comm_topo = MPI_UNDEFINED
      call MPI_Topo_test(dgraph_comm, comm_topo, ierr);
      if (comm_topo .ne. MPI_DIST_GRAPH) then
          validate_dgraph = .false.
          write(6,*) "dgraph_comm is NOT of type MPI_DIST_GRAPH."
          return
      endif

      call MPI_Dist_graph_neighbors_count(dgraph_comm,
     &                                    src_sz, dest_sz, wgt_flag,
     &                                    ierr)
      if (ierr .ne. MPI_SUCCESS) then
          validate_dgraph = .false.
          write(6,*) "MPI_Dist_graph_neighbors_count() fails!"
          return
      endif
      if (.not. wgt_flag) then
          validate_dgraph = .false.
          write(6,*) "dgraph_comm is created with MPI_UNWEIGHTED!"
          return
      endif

      if (src_sz .ne. 2 .or. dest_sz .ne. 2) then
          validate_dgraph = .false.
          write(6,*) "source or destination edge array is not size 2." 
          write(6,"('src_sz = ',I3,', dest_sz = ',I3)") src_sz, dest_sz
          return
      endif

      call MPI_Dist_graph_neighbors(dgraph_comm,
     &                              src_sz, srcs, src_wgts,
     &                              dest_sz, dests, dest_wgts,
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
              write(6,"('srcs[',I3,']=',I3,
     &                  ' is NOT a neighbor of my rank',I3)")
     &              idx, srcs(idx), world_rank
              return
          endif
      enddo
      if (src_wgts(1) .ne. src_wgts(2)) then
          validate_dgraph = .false.
          write(6,"('src_wgts = [',I3,',',I3,']')")
     &          src_wgts(1), src_wgts(2)
          return
      endif
      do idx = 1, dest_sz
          nbr_sep = iabs(dests(idx) - world_rank)
          if (nbr_sep .ne. 1 .and. nbr_sep .ne. (world_size-1)) then
              validate_dgraph = .false.
              write(6,"('dests[',I3,']=',I3,
     &                  ' is NOT a neighbor of my rank',I3)")
     &              idx, dests(idx), world_rank
              return
          endif
      enddo
      if (dest_wgts(1) .ne. dest_wgts(2)) then
          validate_dgraph = .false.
          write(6,"('dest_wgts = [',I3,',',I3,']')")
     &          dest_wgts(1), dest_wgts(2)
          return
      endif

      validate_dgraph = .true.
      return
      end

      integer function ring_rank(world_size, in_rank)
      implicit none
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
      implicit none
      include 'mpif.h'

      integer    ring_rank
      external   ring_rank
      logical    validate_dgraph
      external   validate_dgraph
      integer    errs, ierr

      integer    dgraph_comm
      integer    world_size, world_rank
      integer    src_sz, dest_sz
      integer    degs(1)
      integer    srcs(2), dests(2)
      integer    src_wgts(2), dest_wgts(2)

      errs = 0
      call MTEST_Init(ierr) 
      call MPI_Comm_rank(MPI_COMM_WORLD, world_rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, world_size, ierr)

      srcs(1)      = world_rank
      degs(1)      = 2;
      dests(1)     = ring_rank(world_size, world_rank-1)
      dests(2)     = ring_rank(world_size, world_rank+1)
      dest_wgts(1) = 1
      dest_wgts(2) = 1
      call MPI_Dist_graph_create(MPI_COMM_WORLD, 1, srcs, degs, dests,
     &                           dest_wgts, MPI_INFO_NULL,
     &                          .true., dgraph_comm, ierr)
      if (ierr .ne. MPI_SUCCESS) then
          write(6,*) "MPI_Dist_graph_create() fails!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      if (.not. validate_dgraph(dgraph_comm)) then
          write(6,*) "MPI_Dist_graph_create() does not create "
     &               //"a bidirectional ring graph!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      call MPI_Comm_free(dgraph_comm, ierr)

      src_sz       = 2
      srcs(1)      = ring_rank(world_size, world_rank-1)
      srcs(2)      = ring_rank(world_size, world_rank+1)
      src_wgts(1)  = 1
      src_wgts(2)  = 1
      dest_sz      = 2
      dests(1)     = ring_rank(world_size, world_rank-1)
      dests(2)     = ring_rank(world_size, world_rank+1)
      dest_wgts(1) = 1
      dest_wgts(2) = 1
      call MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD,
     &                                    src_sz, srcs, src_wgts,
     &                                    dest_sz, dests, dest_wgts,
     &                                    MPI_INFO_NULL, .true.,
     &                                    dgraph_comm, ierr)
      if (ierr .ne. MPI_SUCCESS) then
          write(6,*) "MPI_Dist_graph_create_adjacent() fails!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      if (.not. validate_dgraph(dgraph_comm)) then
          write(6,*) "MPI_Dist_graph_create_adjacent() does not create "
     &               //"a bidirectional ring graph!"
          call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
          stop
      endif
      call MPI_Comm_free(dgraph_comm, ierr)

      call MTEST_Finalize(errs)
      call MPI_Finalize(ierr)
      stop
      end
