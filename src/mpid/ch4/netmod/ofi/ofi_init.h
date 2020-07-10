/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_INIT_H_INCLUDED
#define OFI_INIT_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_ofi_version(void)
{
    if (MPIDI_OFI_MAJOR_VERSION != -1 && MPIDI_OFI_MINOR_VERSION != -1)
        return FI_VERSION(MPIDI_OFI_MAJOR_VERSION, MPIDI_OFI_MINOR_VERSION);
    else
        return FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION);
}

int MPIDI_OFI_find_provider(struct fi_info **prov_out);
void MPIDI_OFI_find_provider_cleanup(void);
int MPIDI_OFI_init_multi_nic(struct fi_info *prov);

/* set hints based on MPIDI_OFI_global.settings */
void MPIDI_OFI_init_hints(struct fi_info *hints);

/* set auto progress hints based on CVARs */
void MPIDI_OFI_set_auto_progress(struct fi_info *hints);

/* set MPIDI_OFI_global.settings based on provider-set */
void MPIDI_OFI_init_global_settings(const char *prov_name);

/* whether prov matches MPIDI_OFI_global.settings */
bool MPIDI_OFI_match_global_settings(struct fi_info *prov);

/* update MPIDI_OFI_global.settings */
void MPIDI_OFI_update_global_settings(struct fi_info *prov);

/* Determine if NIC has already been included in others */
bool MPIDI_OFI_nic_already_used(const struct fi_info *prov, struct fi_info **others, int nic_count);

#endif /* OFI_INIT_H_INCLUDED */
