/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_INIT_H_INCLUDED
#define OFI_INIT_H_INCLUDED

int MPIDI_OFI_get_required_version(void);

int MPIDI_OFI_find_provider(struct fi_info **prov_out);
void MPIDI_OFI_find_provider_cleanup(void);
int MPIDI_OFI_init_multi_nic(struct fi_info *prov);

/* set hints based on MPIDI_OFI_global.settings */
void MPIDI_OFI_init_hints(struct fi_info *hints);

/* set auto progress hints based on CVARs */
void MPIDI_OFI_set_auto_progress(struct fi_info *hints);

/* set settings based on provider-set */
void MPIDI_OFI_init_settings(MPIDI_OFI_capabilities_t * p_settings, const char *prov_name);

/* whether prov matches settings, return a score */
int MPIDI_OFI_match_provider(struct fi_info *prov,
                             MPIDI_OFI_capabilities_t * optimal,
                             MPIDI_OFI_capabilities_t * minimal);

/* update MPIDI_OFI_global.settings */
void MPIDI_OFI_update_global_settings(struct fi_info *prov);

/* Determine if NIC has already been included in others */
bool MPIDI_OFI_nic_already_used(const struct fi_info *prov, struct fi_info **others, int nic_count);

int MPIDI_OFI_addr_exchange_root_ctx(void);
int MPIDI_OFI_addr_exchange_all_ctx(void);

bool MPIDI_OFI_nic_is_up(struct fi_info *prov);

#endif /* OFI_INIT_H_INCLUDED */
