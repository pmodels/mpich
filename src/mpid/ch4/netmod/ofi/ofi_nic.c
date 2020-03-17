/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_nic.h"
#include "mpir_hwtopo.h"

/* Determine if NIC has already been included in others */
bool MPIDI_OFI_nic_already_used(const struct fi_info *prov, struct fi_info **others, int nic_count)
{
    for (int i = 0; i < nic_count; ++i) {
        if (prov->nic->bus_attr->bus_type == FI_BUS_PCI &&
            others[i]->nic->bus_attr->bus_type == FI_BUS_PCI) {
            struct fi_pci_attr pci = prov->nic->bus_attr->attr.pci;
            struct fi_pci_attr others_pci = others[i]->nic->bus_attr->attr.pci;
            if (pci.domain_id == others_pci.domain_id && pci.bus_id == others_pci.bus_id &&
                pci.device_id == others_pci.device_id && pci.function_id == others_pci.function_id)
                return true;
        } else {
            if (strcmp(prov->domain_attr->name, others[i]->domain_attr->name) == 0)
                return true;
        }
    }
    return false;
}

/* Setup the multi-NIC data structures to use the fi_info structure given in prov */
void MPIDI_OFI_setup_single_nic(struct fi_info *prov)
{
    MPIDI_OFI_global.prov_use[0] = fi_dupinfo(prov);
    MPIR_Assert(MPIDI_OFI_global.prov_use[0]);
    MPIDI_OFI_global.num_nics = 1;
}

/* TODO: Now that multiple NICs are detected, sort them based on preferred-ness,
 * closeness and count of other processes using the NIC. */
int MPIDI_OFI_setup_multi_nic(void)
{
    int mpi_errno = MPI_SUCCESS;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
