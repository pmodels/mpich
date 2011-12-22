## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## FIXME need to deal with MPID_THREAD_SOURCES junk
## MPID_THREAD_SOURCES was in this list in the simplemake world, but it's
## forbidden in automake: @MPID_THREAD_SOURCES@

lib_lib@MPILIBNAME@_la_SOURCES +=                          \
    src/mpid/ch3/src/ch3u_buffer.c                         \
    src/mpid/ch3/src/ch3u_comm.c                           \
    src/mpid/ch3/src/ch3u_comm_spawn_multiple.c            \
    src/mpid/ch3/src/ch3u_handle_connection.c              \
    src/mpid/ch3/src/ch3u_handle_recv_pkt.c                \
    src/mpid/ch3/src/ch3u_handle_recv_req.c                \
    src/mpid/ch3/src/ch3u_handle_send_req.c                \
    src/mpid/ch3/src/ch3u_port.c                           \
    src/mpid/ch3/src/ch3u_recvq.c                          \
    src/mpid/ch3/src/ch3u_request.c                        \
    src/mpid/ch3/src/ch3u_rma_ops.c                        \
    src/mpid/ch3/src/ch3u_rma_sync.c                       \
    src/mpid/ch3/src/ch3u_rndv.c                           \
    src/mpid/ch3/src/ch3u_eager.c                          \
    src/mpid/ch3/src/ch3u_eagersync.c                      \
    src/mpid/ch3/src/mpid_abort.c                          \
    src/mpid/ch3/src/mpid_cancel_recv.c                    \
    src/mpid/ch3/src/mpid_cancel_send.c                    \
    src/mpid/ch3/src/mpid_comm_disconnect.c                \
    src/mpid/ch3/src/mpid_comm_group_failed.c              \
    src/mpid/ch3/src/mpid_comm_reenable_anysource.c        \
    src/mpid/ch3/src/mpid_comm_spawn_multiple.c            \
    src/mpid/ch3/src/mpid_finalize.c                       \
    src/mpid/ch3/src/mpid_get_universe_size.c              \
    src/mpid/ch3/src/mpid_getpname.c                       \
    src/mpid/ch3/src/mpid_init.c                           \
    src/mpid/ch3/src/mpid_iprobe.c                         \
    src/mpid/ch3/src/mpid_irecv.c                          \
    src/mpid/ch3/src/mpid_irsend.c                         \
    src/mpid/ch3/src/mpid_isend.c                          \
    src/mpid/ch3/src/mpid_issend.c                         \
    src/mpid/ch3/src/mpid_port.c                           \
    src/mpid/ch3/src/mpid_probe.c                          \
    src/mpid/ch3/src/mpid_recv.c                           \
    src/mpid/ch3/src/mpid_rsend.c                          \
    src/mpid/ch3/src/mpid_send.c                           \
    src/mpid/ch3/src/mpid_ssend.c                          \
    src/mpid/ch3/src/mpid_startall.c                       \
    src/mpid/ch3/src/mpid_vc.c                             \
    src/mpid/ch3/src/mpid_rma.c                            \
    src/mpid/ch3/src/mpidi_isend_self.c                    \
    src/mpid/ch3/src/mpidi_pg.c                            \
    src/mpid/ch3/src/mpidi_printf.c

