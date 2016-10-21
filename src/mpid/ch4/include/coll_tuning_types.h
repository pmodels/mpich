#ifndef COLL_TUNING_TYPES_H_INCLUDED
#define COLL_TUNING_TYPES_H_INCLUDED

typedef struct MPIDI_coll_tuning_table_entry {
    int msg_size;
    const void * algo_container;
} MPIDI_coll_tuning_table_entry_t;

typedef struct MPIDI_coll_tuner_table {
    MPIDI_coll_tuning_table_entry_t *table;
    int table_size;
} MPIDI_coll_tuner_table_t;

extern const MPIDI_coll_tuning_table_entry_t BCAST_TUNING_CH4[];
extern const MPIDI_coll_tuning_table_entry_t ALLREDUCE_TUNING_CH4[];
extern const MPIDI_coll_tuning_table_entry_t REDUCE_TUNING_CH4[];

#endif