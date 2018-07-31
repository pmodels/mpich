/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_DEMUX_H_INCLUDED
#define HYDRA_DEMUX_H_INCLUDED

#include "hydra_base.h"

/*! \addtogroup demux Demultiplexing Engine
 * @{
 */

typedef unsigned short HYD_dmx_event_t;

#define HYD_DMX_POLLIN  (0x0001)
#define HYD_DMX_POLLOUT (0x0002)
#define HYD_DMX_POLLHUP (0x0004)

/**
 * \brief HYD_dmx_register_fd - Register file descriptors for events
 *
 * \param[in]  fd       File descriptor
 * \param[in]  events   Events we are interested in
 * \param[in]  userp    Persistent user storage associated with a descriptor
 *                      (not interpreted by the demux engine)
 * \param[in]  callback Callback function to call when the event occurs
 *
 * This function registers all file descriptors to be waited on for an
 * event.
 */
HYD_status HYD_dmx_register_fd(int fd, HYD_dmx_event_t events, void *userp,
                               HYD_status(*callback) (int fd, HYD_dmx_event_t events, void *userp));

/**
 * \brief HYD_dmx_deregister_fd - Deregister file descriptor
 *
 * \param[in]   fd    File descriptor to be deregistered
 *
 * This function deregisters a file descriptor from the demux
 * engine. All registered fd's must be deregistered.
 */
HYD_status HYD_dmx_deregister_fd(int fd);

/**
 * \brief HYD_dmx_wait_for_event - Wait for event
 *
 * \param    time      Time to wait for in seconds (-1 means infinite)
 *
 * This function waits till either one of the registered fd's has had
 * one of its registered events, or till the timeout expires.
 */
HYD_status HYD_dmx_wait_for_event(int wtime);

/**
 * \brief HYD_dmx_query_fd_registration - Query if an fd is registered
 *
 * \param[in]  fd     File descriptor to check whether registered
 *
 * If a file descriptor is being registered and deregistered by
 * different layers, this function allows us to query what its state
 * is. A return value of 1 means that the fd is still registered; 0
 * means that it isn't.
 */
int HYD_dmx_query_fd_registration(int fd);

HYD_status HYD_dmx_stdin_valid(int *out);
HYD_status HYD_dmx_splice(int in, int out);
HYD_status HYD_dmx_unsplice(int fd);

#endif /* HYDRA_DEMUX_H_INCLUDED */
