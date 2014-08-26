#ifndef _HCOLLPRE_H_
#define _HCOLLPRE_H_

typedef struct {
    int is_hcoll_init;
    struct MPID_Collops *hcoll_origin_coll_fns;
    void *hcoll_context;
} hcoll_comm_priv_t;

#endif
