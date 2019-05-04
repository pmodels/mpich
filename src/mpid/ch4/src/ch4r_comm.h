/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef CH4R_COMM_H_INCLUDED
#define CH4R_COMM_H_INCLUDED

int MPIDIU_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                           int **remote_lupids, int *remote_node_ids);
int MPIDIU_Intercomm_map_bcast_intra(MPIR_Comm * local_comm, int local_leader, int *remote_size,
                                     int *is_low_group, int pure_intracomm,
                                     size_t * remote_upid_size, char *remote_upids,
                                     int **remote_lupids, int *remote_node_ids);
int MPIDIU_alloc_lut(MPIDI_rank_map_lut_t ** lut, int size);
int MPIDIU_release_lut(MPIDI_rank_map_lut_t * lut);
int MPIDIU_alloc_mlut(MPIDI_rank_map_mlut_t ** mlut, int size);
int MPIDIU_release_mlut(MPIDI_rank_map_mlut_t * mlut);
int MPIDI_CH4R_mpi_comm_set_info_mutable(MPIR_Comm * comm, MPIR_Info * info);
int MPIDI_CH4R_mpi_comm_set_info_immutable(MPIR_Comm * comm, MPIR_Info * info);
int MPIDI_CH4R_mpi_comm_get_info(MPIR_Comm * comm, MPIR_Info ** info_p_p);
bool MPIDIU_parse_string_to_bool(const char *value, bool * parsed_val);

#endif /* CH4R_COMM_H_INCLUDED */
