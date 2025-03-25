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
        If an address is used more than once in the last ten send operations,
        map it for IPC use even if it is below the IPC threshold.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPIDI_IPCI_global_t MPIDI_IPCI_global;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_IPCI_DBG_GENERAL;
#endif

bool MPIDI_IPCI_is_repeat_addr(void *addr)
{
    if (!MPIR_CVAR_CH4_IPC_MAP_REPEAT_ADDR) {
        return false;
    }

    static void *repeat_addr[10] = { 0 };
    static int addr_idx = 0;

    for (int i = 0; i < 10; i++) {
        if (addr == repeat_addr[i]) {
            return true;
        }
    }

    repeat_addr[addr_idx] = addr;
    addr_idx = (addr_idx + 1) % 10;
    return false;
}
