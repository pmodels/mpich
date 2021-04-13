/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_NIC_H_INCLUDED
#define OFI_NIC_H_INCLUDED

/* Determine if NIC has already been included in others */
bool MPIDI_OFI_nic_already_used(const struct fi_info *prov, struct fi_info **others, int nic_count);

/* Setup the multi-NIC data structures to use the fi_info structure given in prov */
void MPIDI_OFI_setup_single_nic(struct fi_info *prov);

int MPIDI_OFI_setup_multi_nic(void);

#endif /* OFI_NIC_H_INCLUDED */
