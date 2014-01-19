/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 NEC Corporation
 *      Author: Masamichi Takagi
 *  (C) 2012 Oct 10 Min Si
 *  (C) 2001-2009 Yutaka Ishikawa
 *      See COPYRIGHT in top-level directory.
 */

#include <unistd.h>
#include <stdlib.h>
#include "ib_ibcom.h"

//#define MPID_NEM_IB_DEBUG_REG_MR
#ifdef MPID_NEM_IB_DEBUG_REG_MR
#define dprintf printf
#else
#define dprintf(...)
#endif

/* cache size of ibv_reg_mr */
#define MPID_NEM_IB_COM_REG_MR_NLINE 4096
#define MPID_NEM_IB_COM_REG_MR_NWAY  1024

#define MPID_NEM_IB_COM_REG_MR_SZPAGE 4096
#define MPID_NEM_IB_COM_REG_MR_LOGSZPAGE 12

/* arena allocator */

#define MPID_NEM_IB_NIALLOCID 32
typedef struct {
    char *next;
} free_list_t;
static char *free_list_front[MPID_NEM_IB_NIALLOCID] = { 0 };
static char *arena_flist[MPID_NEM_IB_NIALLOCID] = { 0 };

#define MPID_NEM_IB_SZARENA 4096
#define MPID_NEM_IB_CLUSTER_SIZE (MPID_NEM_IB_SZARENA/sz)
#define MPID_NEM_IB_ROUNDUP64(addr, align) ((addr + align - 1) & ~((unsigned long)align - 1))
#define MPID_NEM_IB_NCLUST_SLAB 1
#define MPID_NEM_IB_COM_AALLOC_ID_MRCACHE 0

static inline void *aalloc(size_t sz, int id)
{
#if 1   /* debug */
    return MPIU_Malloc(sz);
#else
    char *p = free_list_front[id];
    if ((unsigned long) p & (MPID_NEM_IB_SZARENA - 1)) {
        free_list_front[id] += sz;
        return p;
    }
    else {
        char *q, r;
        if (arena_flist[id]) {
            q = arena_flist[id];
            arena_flist[id] = ((free_list_t *) arena_flist[id])->next;
        }
        else {
            q = mmap(NULL,
                     MPID_NEM_IB_ROUNDUP64(MPID_NEM_IB_SZARENA * MPID_NEM_IB_NCLUST_SLAB, 4096),
                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#if MPID_NEM_IB_NCLUST_SLAB > 1
            arena_flist[id] = q + MPID_NEM_IB_SZARENA;
            for (p = arena_flist[id]; p < q + (MPID_NEM_IB_NCLUST_SLAB - 1) * MPID_NEM_IB_SZARENA;
                 p += MPID_NEM_IB_SZARENA) {
                ((free_list_t *) p)->next = p + MPID_NEM_IB_SZARENA;
            }
            ((free_list_t *) p)->next = 0;
#endif
        }
        *((int *) q) = MPID_NEM_IB_CLUSTER_SIZE - 1;
        //      dprintf("q=%llx\n", q);
        q += sz + (MPID_NEM_IB_SZARENA % sz);
        free_list_front[id] = q + sz;
        return q;
    }
#endif
}

static inline void afree(const void *p, int id)
{
#if 1   /* debug */
    return MPIU_Free((void *) p);
#else
    p = (void *) ((unsigned long) p & ~(MPID_NEM_IB_SZARENA - 1));
    if (!(--(*((int *) p)))) {
        ((free_list_t *) p)->next = arena_flist[id];
        arena_flist[id] = (char *) p;
    }
#endif
}

struct MPID_nem_ib_com_reg_mr_listnode_t {
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_next;
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_prev;
};

struct MPID_nem_ib_com_reg_mr_cache_entry_t {
    /* : public MPID_nem_ib_com_reg_mr_listnode_t */
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_next;
    struct MPID_nem_ib_com_reg_mr_listnode_t *lru_prev;

    struct ibv_mr *mr;
    void *addr;
    int len;
    int refc;
};

static struct MPID_nem_ib_com_reg_mr_listnode_t
    MPID_nem_ib_com_reg_mr_cache[MPID_NEM_IB_COM_REG_MR_NLINE];

__inline__ int MPID_nem_ib_com_hash_func(char *addr)
{
    unsigned int v = (unsigned int) (unsigned long) addr;
    //v = v >> MPID_NEM_IB_COM_REG_MR_LOGSZPAGE; /* assume it is page aligned */
    v = v & (MPID_NEM_IB_COM_REG_MR_NLINE - 1);
    return (int) v;
}

static void MPID_nem_ib_com_reg_mr_insert(struct MPID_nem_ib_com_reg_mr_listnode_t *c,
                                          struct MPID_nem_ib_com_reg_mr_listnode_t *e)
{
    struct MPID_nem_ib_com_reg_mr_listnode_t *next;
    struct MPID_nem_ib_com_reg_mr_listnode_t *prev;
    prev = c;
    next = prev->lru_next;
    e->lru_next = next;
    e->lru_prev = prev;
    next->lru_prev = e;
    prev->lru_next = e;
}

static void MPID_nem_ib_com_reg_mr_unlink(struct MPID_nem_ib_com_reg_mr_listnode_t *e)
{
    struct MPID_nem_ib_com_reg_mr_listnode_t *next, *prev;
    next = e->lru_next;
    prev = e->lru_prev;
    next->lru_prev = prev;
    prev->lru_next = next;
}

static inline void __lru_queue_display()
{
    struct MPID_nem_ib_com_reg_mr_cache_entry_t *p;
    int i = 0;
    for (i = 0; i < MPID_NEM_IB_COM_REG_MR_NLINE; i++) {
        dprintf("---- hash %d\n", i);
        for (p =
             (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) MPID_nem_ib_com_reg_mr_cache[i].
             lru_next;
             p != (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) &MPID_nem_ib_com_reg_mr_cache[i];
             p = (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) p->lru_next) {
            if (p && p->addr) {
                dprintf("-------- p=%p,addr=%p,len=%d,refc=%d,lru_next=%p\n", p, p->addr, p->len,
                        p->refc, p->lru_next);
            }
            else {
                dprintf("-------- p=%p,lru_next=%p\n", p, p->lru_next);
            }
        }
    }
}

struct ibv_mr *MPID_nem_ib_com_reg_mr_fetch(void *addr, int len)
{
#if 0   /* debug */
    struct ibv_mr *mr;
    int ibcom_errno = MPID_nem_ib_com_reg_mr(addr, len, &mr);
    printf("mrcache,MPID_nem_ib_com_reg_mr,error,addr=%p,len=%d,lkey=%08x,rkey=%08x\n", addr, len,
           mr->lkey, mr->rkey);
    if (ibcom_errno != 0) {
        goto fn_fail;
    }
  fn_exit:
    return mr;
  fn_fail:
    goto fn_exit;
#else
    int ibcom_errno;
    int key;
    struct MPID_nem_ib_com_reg_mr_cache_entry_t *e;

#if 1   /*def HAVE_LIBDCFA */
    /* we can't change addr because ibv_post_send assumes mr->host_addr (output of this function)
     * must have an exact mirror value of addr (input of this function) */
    void *addr_aligned = addr;
    int len_aligned = len;
#else
    void *addr_aligned = (void *) ((unsigned long) addr & ~(MPID_NEM_IB_COM_REG_MR_SZPAGE - 1));
    int len_aligned =
        ((((unsigned long) addr + len) - (unsigned long) addr_aligned +
          MPID_NEM_IB_COM_REG_MR_SZPAGE - 1) & ~(MPID_NEM_IB_COM_REG_MR_SZPAGE - 1));
#endif
    key = MPID_nem_ib_com_hash_func(addr);

    dprintf("[MrCache] addr=%p, len=%d\n", addr, len);
    dprintf("[MrCache] aligned addr=%p, len=%d\n", addr_aligned, len_aligned);

    //__lru_queue_display();
    int way = 0;
    for (e =
         (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) MPID_nem_ib_com_reg_mr_cache[key].lru_next;
         e != (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) &MPID_nem_ib_com_reg_mr_cache[key];
         e = (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) e->lru_next, way++) {
        //dprintf("e=%p, e->hash_next=%p\n", e, e->lru_next);

        if (e->addr <= addr_aligned &&
            (uint8_t *) addr_aligned + len_aligned <= (uint8_t *) e->addr + e->len) {
            //dprintf
            //("MPID_nem_ib_com_reg_mr_fetch,hit,entry addr=%p,len=%d,mr addr=%p,len=%ld,requested addr=%p,len=%d\n",
            //e->addr, e->len, e->mr->addr, e->mr->length, addr, len);
            goto hit;
        }
    }

    // miss

    // evict an entry and de-register its MR when the cache-set is full
    if (way > MPID_NEM_IB_COM_REG_MR_NWAY) {
        struct MPID_nem_ib_com_reg_mr_cache_entry_t *victim =
            (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) e->lru_prev;
        MPID_nem_ib_com_reg_mr_unlink((struct MPID_nem_ib_com_reg_mr_listnode_t *) victim);

        //dprintf("MPID_nem_ib_com_reg_mr,evict,entry addr=%p,len=%d,mr addr=%p,len=%ld\n", e->addr, e->len,
        //e->mr->addr, e->mr->length);
        ibcom_errno = MPID_nem_ib_com_dereg_mr(victim->mr);
        if (ibcom_errno) {
            printf("mrcache,MPID_nem_ib_com_dereg_mr\n");
            goto fn_fail;
        }
        afree(victim, MPID_NEM_IB_COM_AALLOC_ID_MRCACHE);
    }

    e = aalloc(sizeof(struct MPID_nem_ib_com_reg_mr_cache_entry_t),
               MPID_NEM_IB_COM_AALLOC_ID_MRCACHE);
    /* reference counter is used when evicting entry */
    e->refc = 1;

    dprintf("MPID_nem_ib_com_reg_mr_fetch,miss,addr=%p,len=%d\n", addr_aligned, len_aligned);
    /* register memory */
    ibcom_errno = MPID_nem_ib_com_reg_mr(addr_aligned, len_aligned, &e->mr);
    if (ibcom_errno != 0) {
        fprintf(stderr, "mrcache,MPID_nem_ib_com_reg_mr\n");
        goto fn_fail;
    }
    e->addr = addr_aligned;
    e->len = len_aligned;

    //dprintf("MPID_nem_ib_com_reg_mr_fetch,fill,e=%p,key=%d,mr=%p,mr addr=%p,len=%ld,lkey=%08x,rkey=%08x\n", e,
    //key, e->mr, e->mr->addr, e->mr->length, e->mr->lkey, e->mr->rkey);

    /* register to cache */
    MPID_nem_ib_com_reg_mr_insert(&MPID_nem_ib_com_reg_mr_cache[key],
                                  (struct MPID_nem_ib_com_reg_mr_listnode_t *) e);

    //__lru_queue_display();

    goto fn_exit;

  hit:

    /* reference counter is used when evicting entry */
    e->refc++;
#if 0   /* disable for debug */
    /* move to head of the list */
    if (e !=
        (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) MPID_nem_ib_com_reg_mr_cache[key].
        lru_next) {
        MPID_nem_ib_com_reg_mr_unlink((struct MPID_nem_ib_com_reg_mr_listnode_t *) e);
        MPID_nem_ib_com_reg_mr_insert(&MPID_nem_ib_com_reg_mr_cache[key],
                                      (struct MPID_nem_ib_com_reg_mr_listnode_t *) e);
    }
#endif
    //dprintf("[MrCache] reuse e=%p,key=%d,mr=%p,refc=%d,addr=%p,len=%ld,lkey=%08x,rkey=%08x\n", e,
    //key, e->mr, e->refc, e->mr->addr, e->mr->length, e->mr->lkey, e->mr->rkey);

    //__lru_queue_display();

  fn_exit:
    return e->mr;
  fn_fail:
    goto fn_exit;
#endif
}

static void MPID_nem_ib_com_reg_mr_dereg(struct ibv_mr *mr)
{

    struct MPID_nem_ib_com_reg_mr_cache_entry_t *e;
    struct MPID_nem_ib_com_reg_mr_cache_entry_t *zero = 0;
    unsigned long offset = (unsigned long) zero->mr;
    e = (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) ((unsigned long) mr - offset);
    e->refc--;

    //dprintf("MPID_nem_ib_com_reg_mr_dereg,entry=%p,mr=%p,addr=%p,refc=%d,offset=%lx\n", e, mr, e->mr->addr,
    //e->refc, offset);
}

void MPID_nem_ib_com_register_cache_init()
{
    int i;

    /* Using the address to the start node to express the end of the list
     * instead of using NULL */
    for (i = 0; i < MPID_NEM_IB_COM_REG_MR_NLINE; i++) {
        MPID_nem_ib_com_reg_mr_cache[i].lru_next =
            (struct MPID_nem_ib_com_reg_mr_listnode_t *) &MPID_nem_ib_com_reg_mr_cache[i];
        MPID_nem_ib_com_reg_mr_cache[i].lru_prev =
            (struct MPID_nem_ib_com_reg_mr_listnode_t *) &MPID_nem_ib_com_reg_mr_cache[i];
    }

    dprintf("[MrCache] cache initializes %d entries\n", MPID_NEM_IB_COM_REG_MR_NLINE);
}

void MPID_nem_ib_com_register_cache_destroy()
{
    struct MPID_nem_ib_com_reg_mr_cache_entry_t *p;
    int i = 0, cnt = 0;

    for (i = 0; i < MPID_NEM_IB_COM_REG_MR_NLINE; i++) {
        for (p =
             (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) MPID_nem_ib_com_reg_mr_cache[i].
             lru_next;
             p != (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) &MPID_nem_ib_com_reg_mr_cache[i];
             p = (struct MPID_nem_ib_com_reg_mr_cache_entry_t *) p->lru_next) {
            if (p && p->addr > 0) {
                MPID_nem_ib_com_dereg_mr(p->mr);
                afree(p, MPID_NEM_IB_COM_AALLOC_ID_MRCACHE);
                cnt++;
            }
        }
    }

    //__lru_queue_display();

    dprintf("[MrCache] cache destroyed %d entries\n", cnt);
}
