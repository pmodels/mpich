/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DEMUX_H_INCLUDED
#define DEMUX_H_INCLUDED

/** @file demux.h */

#include "hydra.h"

/*! \addtogroup demux Demultiplexing Engine
 * @{
 */

/**
 * \brief HYDT_dmx_register_fd - Register file descriptors for events
 *
 * \param[in]  num_fds  Number of file descriptors being provided
 * \param[in]  fd       Array of file descriptors
 * \param[in]  events   Events we are interested in
 * \param[in]  userp    Persistent user storage associated with a descriptor
 *                      (not interpreted by the demux engine)
 * \param[in]  callback Callback function to call when the event occurs
 *
 * This function registers all file descriptors to be waited on for an
 * event.
 */
HYD_status HYDT_dmx_register_fd(int num_fds, int *fd, HYD_event_t events, void *userp,
                                HYD_status(*callback) (int fd, HYD_event_t events,
                                                       void *userp));

/**
 * \brief HYDT_dmx_deregister_fd - Deregister file descriptor
 *
 * \param[in]   fd    File descriptor to be deregistered
 *
 * This function deregisters a file descriptor from the demux
 * engine. All registered fd's must be deregistered.
 */
HYD_status HYDT_dmx_deregister_fd(int fd);

/**
 * \brief HYDT_dmx_wait_for_event - Wait for event
 *
 * \param    time      Time to wait for in millisecond (-1 means infinite)
 *
 * This function waits till either one of the registered fd's has had
 * one of its registered events, or till the timeout expires.
 */
HYD_status HYDT_dmx_wait_for_event(int time);

/**
 * \brief HYDT_dmx_finalize - Finalize demux engine
 *
 * This function cleans up any relevant state that the demux engine
 * maintained.
 */
HYD_status HYDT_dmx_finalize(void);

/*!
 * @}
 */

#endif /* DEMUX_H_INCLUDED */
