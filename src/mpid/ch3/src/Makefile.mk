## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_core_sources +=                          \
    src/mpid/ch3/src/ch3u_buffer.c                         \
    src/mpid/ch3/src/ch3u_comm.c                           \
    src/mpid/ch3/src/ch3u_comm_spawn_multiple.c            \
    src/mpid/ch3/src/ch3u_handle_connection.c              \
    src/mpid/ch3/src/ch3u_handle_recv_pkt.c                \
    src/mpid/ch3/src/ch3u_handle_recv_req.c                \
    src/mpid/ch3/src/ch3u_handle_revoke_pkt.c              \
    src/mpid/ch3/src/ch3u_handle_send_req.c                \
    src/mpid/ch3/src/ch3u_handle_op_req.c                  \
    src/mpid/ch3/src/ch3u_port.c                           \
    src/mpid/ch3/src/ch3u_recvq.c                          \
    src/mpid/ch3/src/ch3u_request.c                        \
    src/mpid/ch3/src/ch3u_rma_progress.c                   \
    src/mpid/ch3/src/ch3u_rma_ops.c                        \
    src/mpid/ch3/src/ch3u_rma_reqops.c                     \
    src/mpid/ch3/src/ch3u_rma_sync.c                       \
    src/mpid/ch3/src/ch3u_rma_pkthandler.c                 \
    src/mpid/ch3/src/ch3u_rndv.c                           \
    src/mpid/ch3/src/ch3u_eager.c                          \
    src/mpid/ch3/src/ch3u_eagersync.c                      \
    src/mpid/ch3/src/ch3u_win_fns.c                        \
    src/mpid/ch3/src/mpid_abort.c                          \
    src/mpid/ch3/src/mpid_cancel_recv.c                    \
    src/mpid/ch3/src/mpid_cancel_send.c                    \
    src/mpid/ch3/src/mpid_comm_disconnect.c                \
    src/mpid/ch3/src/mpid_comm_spawn_multiple.c            \
    src/mpid/ch3/src/mpid_comm_failure_ack.c               \
    src/mpid/ch3/src/mpid_comm_get_all_failed_procs.c      \
    src/mpid/ch3/src/mpid_comm_revoke.c                    \
    src/mpid/ch3/src/mpid_finalize.c                       \
    src/mpid/ch3/src/mpid_get_universe_size.c              \
    src/mpid/ch3/src/mpid_getpname.c                       \
    src/mpid/ch3/src/mpid_improbe.c                        \
    src/mpid/ch3/src/mpid_imrecv.c                         \
    src/mpid/ch3/src/mpid_init.c                           \
    src/mpid/ch3/src/mpid_iprobe.c                         \
    src/mpid/ch3/src/mpid_irecv.c                          \
    src/mpid/ch3/src/mpid_irsend.c                         \
    src/mpid/ch3/src/mpid_isend.c                          \
    src/mpid/ch3/src/mpid_issend.c                         \
    src/mpid/ch3/src/mpid_mprobe.c                         \
    src/mpid/ch3/src/mpid_mrecv.c                          \
    src/mpid/ch3/src/mpid_port.c                           \
    src/mpid/ch3/src/mpid_probe.c                          \
    src/mpid/ch3/src/mpid_recv.c                           \
    src/mpid/ch3/src/mpid_rsend.c                          \
    src/mpid/ch3/src/mpid_send.c                           \
    src/mpid/ch3/src/mpid_ssend.c                          \
    src/mpid/ch3/src/mpid_startall.c                       \
    src/mpid/ch3/src/mpid_vc.c                             \
    src/mpid/ch3/src/mpid_rma.c                            \
    src/mpid/ch3/src/mpidi_rma.c                           \
    src/mpid/ch3/src/mpid_aint.c                           \
    src/mpid/ch3/src/mpidi_isend_self.c                    \
    src/mpid/ch3/src/mpidi_pg.c                            \
    src/mpid/ch3/src/mpidi_printf.c

