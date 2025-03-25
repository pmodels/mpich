/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ipc_types.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_IPC_MAP_REPEAT_ADDR
      category    : CH4
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable to track how often a buffer is being sent repeatedly. This will
        be used in determine whether to use IPC algorithm to deliver the message.
        The choice will depend on the IPC driver. In the case of high-latency
        buffers such as GPU device buffer, we will enable IPC if EITHER the
        message size is above a threshold or the message buffer is being repeated.
        On the other hand, if the address mapping overhead is relatively high,
        such as the case for XPMEM, we will enable IPC when BOTH conditions --
        message size and repeat count -- are met.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPIDI_IPCI_global_t MPIDI_IPCI_global;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_IPCI_DBG_GENERAL;
#endif

int MPIDI_IPCI_is_repeat_addr(const void *addr)
{
    if (!MPIR_CVAR_CH4_IPC_MAP_REPEAT_ADDR) {
        return 0;
    }
#define MAX_REPEAT_ADDRS 16     /* a bit arbitrary - TODO: make it tunable */
    static const void *repeat_addr[MAX_REPEAT_ADDRS] = { 0 };
    static int repeat_addr_count[MAX_REPEAT_ADDRS];
    static int addr_idx = 0;

    for (int i = 0; i < MAX_REPEAT_ADDRS; i++) {
        if (addr == repeat_addr[i]) {
            repeat_addr_count[i]++;
            return repeat_addr_count[i];
        }
    }

    repeat_addr[addr_idx] = addr;
    repeat_addr_count[addr_idx] = 0;
    addr_idx = (addr_idx + 1) % MAX_REPEAT_ADDRS;
    return 0;
}
