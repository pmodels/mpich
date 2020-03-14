/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

#endif /* CH4R_COMM_H_INCLUDED */
