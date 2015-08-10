#ifndef OFI_TAG_LAYOUT_H
#define OFI_TAG_LAYOUT_H




/* *******************************************************************************
 *  match/ignore bit manipulation
 * *******************************************************************************
 * 01234567 01234567 | 01234567 01234567 | 0123 | 4567 01234567 01234567 01234567
 *                   |                   | proto|                                |
 *       source      |       ctxid       |  col |           message tag          |
 *
 *********************************************************************************/
#define MPID_PROTOCOL_MASK       (0x00000000F0000000ULL)
#define MPID_CONTEXT_MASK        (0x0000FFFF00000000ULL)
#define MPID_SOURCE_MASK         (0xFFFF000000000000ULL)
#define MPID_TAG_MASK            (0x000000000FFFFFFFULL)
#define MPID_PGID_MASK           (0xFFFFFFFF00000000ULL)
#define MPID_PSOURCE_MASK        (0x000000000000FFFFULL)
#define MPID_PORT_NAME_MASK      (0x000000000000FFFFULL)
#define MPID_SYNC_SEND           (0x0000000080000000ULL)
#define MPID_SYNC_SEND_ACK       (0x0000000090000000ULL)
#define MPID_MSG_RTS             (0x0000000010000000ULL)
#define MPID_MSG_CTS             (0x0000000020000000ULL)
#define MPID_MSG_DATA            (0x0000000040000000ULL)
#define MPID_CONN_REQ            (0x00000000A0000000ULL)
#define MPID_SOURCE_SHIFT        (48)
#define MPID_CTXID_SHIFT         (32)
#define MPID_PGID_SHIFT          (32)
#define MPID_PSOURCE_SHIFT       (0)
#define MPID_PORT_SHIFT          (0)
#define MPID_TAG_BITS            (28)
#define MPID_RANK_BITS           (16)
#define MPID_RCD_IGNORE_MASK     (0xFFFFFFFF0FFFFFFFULL)
#define OFI_KVSAPPSTRLEN         1024


/* ******************************** */
/* Tag Manipulation inlines         */
/* ******************************** */
static inline uint64_t init_sendtag(MPIU_Context_id_t contextid, int source, int tag, uint64_t type)
{
    uint64_t match_bits = 0;
    match_bits |= ((uint64_t)source) << MPID_SOURCE_SHIFT;
    match_bits |= ((uint64_t)contextid) << MPID_CTXID_SHIFT;
    match_bits |= (MPID_TAG_MASK & tag) | type;
    return match_bits;
}

/* receive posting */
static inline uint64_t init_recvtag(uint64_t * mask_bits,
                                    MPIU_Context_id_t contextid, int source, int tag)
{
    uint64_t match_bits = 0;
    *mask_bits = MPID_SYNC_SEND;
    match_bits |= ((uint64_t)contextid) << MPID_CTXID_SHIFT;

    if (MPI_ANY_SOURCE == source) {
        *mask_bits |= MPID_SOURCE_MASK;
    }
    else {
        match_bits |= ((uint64_t)source) << MPID_SOURCE_SHIFT;
    }
    if (MPI_ANY_TAG == tag)
        *mask_bits |= MPID_TAG_MASK;
    else
        match_bits |= (MPID_TAG_MASK & tag);

    return match_bits;
}

static inline int get_tag(uint64_t match_bits)
{
    return ((int) (match_bits & MPID_TAG_MASK));
}

static inline int get_source(uint64_t match_bits)
{
    return ((int) ((match_bits & MPID_SOURCE_MASK) >> (MPID_SOURCE_SHIFT)));
}

static inline int get_psource(uint64_t match_bits)
{
    return ((int) ((match_bits & MPID_PSOURCE_MASK) >> (MPID_PSOURCE_SHIFT)));
}

static inline int get_pgid(uint64_t match_bits)
{
    return ((int) ((match_bits & MPID_PGID_MASK) >> MPID_PGID_SHIFT));
}

static inline int get_port(uint64_t match_bits)
{
    return ((int) ((match_bits & MPID_PORT_NAME_MASK) >> MPID_PORT_SHIFT));
}


/*********************************************************************************
 *              SECOND MODE TAG LAUOUT MACROS                                    *
 *********************************************************************************/

/*********************************************************************************
 * 01234567 01234567 | 01234567 01234567 | 0123 | 4567 01234567 01234567 01234567
 *                   |                   | proto|                                |
 *       Not used    |       ctxid       |  col |           message tag          |
 *
 *********************************************************************************/
#define MPID_RCD_IGNORE_MASK_M2     (0x000000000FFFFFFFULL)


/* ******************************** */
/* Tag Manipulation inlines         */
/* ******************************** */
static inline uint64_t init_sendtag_2(MPIU_Context_id_t contextid, int tag, uint64_t type)
{
    uint64_t match_bits = 0;
    match_bits |= ((uint64_t)contextid) << MPID_CTXID_SHIFT;
    match_bits |= (MPID_TAG_MASK & tag) | type;
    return match_bits;
}

/* receive posting */
static inline uint64_t init_recvtag_2(uint64_t * mask_bits,
                                    MPIU_Context_id_t contextid, int tag)
{
    uint64_t match_bits = 0;
    *mask_bits = MPID_SYNC_SEND;
    match_bits |= ((uint64_t)contextid) << MPID_CTXID_SHIFT;

    if (MPI_ANY_TAG == tag)
        *mask_bits |= MPID_TAG_MASK;
    else
        match_bits |= (MPID_TAG_MASK & tag);

    return match_bits;
}

#define GET_RCD_IGNORE_MASK() (gl_data.api_set == API_SET_1 ? \
                               MPID_RCD_IGNORE_MASK : MPID_RCD_IGNORE_MASK_M2)
#define API_SET_1 1
#define API_SET_2 2

#endif /*OFI_TAG_LAYOUT_H*/
