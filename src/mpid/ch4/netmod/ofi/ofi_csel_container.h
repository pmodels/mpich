#ifndef OFI_CSEL_CONTAINER_H_INCLUDED
#define OFI_CSEL_CONTAINER_H_INCLUDED

typedef enum {
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Barrier_intra_triggered_tagged,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Barrier_intra_switch_offload,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_impl,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Ibarrier_intra_switch_offload,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_sched_intra_tsp_auto,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Bcast_intra_triggered_tagged,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Bcast_intra_triggered_rma,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Bcast_intra_triggered_pipelined,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Bcast_intra_triggered_small_blocking,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Bcast_intra_switch_offload,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_impl,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Ibcast_intra_switch_offload,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_sched_intra_tsp_auto,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Allreduce_intra_triggered_tagged,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Allreduce_intra_triggered_rma,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Allreduce_intra_triggered_pipelined,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Allreduce_intra_triggered_tree_small_message,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Allreduce_intra_switch_offload,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_impl,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_OFI_Iallreduce_intra_switch_offload,
    MPIDI_OFI_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_sched_intra_tsp_auto,
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
        } barrier;
        struct {
            struct {
                int k;
                int tree_type;
            } triggered_tagged;
            struct {
                int k;
                int tree_type;
            } triggered_rma;
            struct {
                int k;
                int tree_type;
                int chunk_size;
            } triggered_pipelined;
            struct {
                int k;
            } triggered_small_blocking;
        } bcast;
        struct {
            struct {
                int k;
                int tree_type;
            } triggered_tagged;
            struct {
                int k;
                int tree_type;
            } triggered_rma;
            struct {
                int k;
                int chunk_size;
            } triggered_pipelined;
            struct {
                int k;
            } triggered_tree_small_message;
        } allreduce;

    } u;
} MPIDI_OFI_csel_container_s;

#endif /* OFI_CSEL_CONTAINER_H_INCLUDED */
