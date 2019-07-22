/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CH4_VCI_H_INCLUDED
#define CH4_VCI_H_INCLUDED

#include "ch4_impl.h"

 /** Slow-path function declarations **/
#ifndef MPIDI_CH4_DIRECT_NETMOD
int MPIDI_vci_pool_alloc(int num_vsis, int num_vnis);
#else
int MPIDI_vci_pool_alloc(int num_vnis);
#endif
int MPIDI_vci_pool_free(void);
int MPIDI_vci_alloc(MPIDI_vci_type_t type, MPIDI_vci_resource_t resources,
                    MPIDI_vci_property_t properties, int *vci);
int MPIDI_vci_free(int vci);

/** Fast-path functions **/
#if MPIDI_CH4_VCI_METHOD == MPICH_VCI__ZERO
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return 0;
}
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return 0;
}
#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__COMM
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    /* Only single  VCI per comm */
    return MPIDI_COMM_VCI(comm_ptr);
}
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    /* Only single  VCI per comm */
    return MPIDI_COMM_VCI(comm_ptr);
}
#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__TAG
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    /* bit 10-14 */
    return (tag == MPI_ANY_TAG) ? 0 : ((tag & 0x7c00) >> 10) % MPIDI_VCI_POOL(max_vcis);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    /* bit 5-9 */
    return (tag == MPI_ANY_TAG) ? 0 : ((tag & 0x3e0) >> 5) % MPIDI_VCI_POOL(max_vcis);
}
#elif MPIDI_CH4_VCI_METHOD == MPICH_VCI__TLS
#ifdef MPL_TLS
extern MPL_TLS int MPIDI_vci_src;
extern MPL_TLS int MPIDI_vci_dst;
#else
extern int MPIDI_vci_src;
extern int MPIDI_vci_dst;
#endif
MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return MPIDI_vci_src;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_vci_get_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return MPIDI_vci_dst;
}
#else
#error Invalid MPIDI_CH4_VCI_METHOD!
#endif

#endif /* CH4_VCI_H_INCLUDED */
