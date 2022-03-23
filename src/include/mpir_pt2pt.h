/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_PT2PT_H_INCLUDED
#define MPIR_PT2PT_H_INCLUDED

/* NOTE: All explicit vci (allocated) must be greater than 0 */

#define MPIR_PT2PT_ATTR_SRC_VCI_SHIFT 8
#define MPIR_PT2PT_ATTR_DST_VCI_SHIFT 16

#define MPIR_PT2PT_ATTR_HAS_VCI(attr) ((attr) & 0xffff00)
#define MPIR_PT2PT_ATTR_SRC_VCI(attr) (((attr) >> MPIR_PT2PT_ATTR_SRC_VCI_SHIFT) & 0xff)
#define MPIR_PT2PT_ATTR_DST_VCI(attr) (((attr) >> MPIR_PT2PT_ATTR_DST_VCI_SHIFT) & 0xff)
#define MPIR_PT2PT_ATTR_SET_VCIS(attr, src_vci, dst_vci) \
    do { \
        (attr) |= ((src_vci) << MPIR_PT2PT_ATTR_SRC_VCI_SHIFT) | ((dst_vci) << MPIR_PT2PT_ATTR_DST_VCI_SHIFT); \
    } while (0)

/* bit 0: context_offset */
#define MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr) ((attr) & 0x01)
#define MPIR_PT2PT_ATTR_SET_CONTEXT_OFFSET(attr, context_offset) (attr) |= (context_offset)

int MPIR_Ibsend_impl(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                     MPIR_Comm * comm_ptr, MPI_Request * request);

#endif /* MPIR_PT2PT_H_INCLUDED */
