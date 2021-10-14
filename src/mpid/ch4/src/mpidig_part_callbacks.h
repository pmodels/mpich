/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_PART_CALLBACKS_H_INCLUDED
#define MPIDIG_PART_CALLBACKS_H_INCLUDED

int MPIDIG_part_send_data_origin_cb(MPIR_Request * req);

int MPIDIG_part_send_init_target_msg_cb(void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDIG_part_cts_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  uint32_t attr, MPIR_Request ** req);
int MPIDIG_part_send_data_target_msg_cb(void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);

#endif /* MPIDIG_PART_CALLBACKS_H_INCLUDED */
