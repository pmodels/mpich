#ifndef OFI_CSEL_CONTAINER_H_INCLUDED
#define OFI_CSEL_CONTAINER_H_INCLUDED

typedef enum {
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Bcast_intra_triggered_tagged,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Bcast_intra_triggered_rma,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_impl,
    MPIDI_OFI_Algorithm_count,
} MPIDI_OFI_Csel_container_type_e;

typedef struct {
    MPIDI_OFI_Csel_container_type_e id;

    union {
        struct {
            struct {
                int k;
                int tree_type;
            } triggered_tagged;
            struct {
                int k;
                int tree_type;
            } triggered_rma;
        } bcast;
    } u;
} MPIDI_OFI_csel_container_s;

#endif /* OFI_CSEL_CONTAINER_H_INCLUDED */
