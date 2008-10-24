/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DEMUX_H_INCLUDED
#define DEMUX_H_INCLUDED

#include "hydra.h"
#include "csi.h"

HYD_Status HYD_DMX_Register_fd(int num_fds, int * fd, HYD_CSI_Event_t events,
			       HYD_Status (*callback) (int fd, HYD_CSI_Event_t events));
HYD_Status HYD_DMX_Deregister_fd(int fd);
HYD_Status HYD_DMX_Wait_for_event();
HYD_Status HYD_DMX_Finalize();

#endif /* DEMUX_H_INCLUDED */
