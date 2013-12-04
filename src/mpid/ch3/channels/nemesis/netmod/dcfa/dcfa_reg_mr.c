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
#include "dcfa_ibcom.h"

//#define DEBUG_REG_MR
#ifdef DEBUG_REG_MR
#define dprintf printf
#else
#define dprintf(...)
#endif

/* cache size of ibv_reg_mr */
#define IBCOM_REG_MR_NLINE 4096
#define IBCOM_REG_MR_NWAY  1024

#define IBCOM_REG_MR_SZPAGE 4096
#define IBCOM_REG_MR_LOGSZPAGE 12

/* arena allocator */

#define NIALLOCID 32
typedef struct {
    char *next;
} free_list_t;
static char *free_list_front[NIALLOCID] = { 0 };
static char *arena_flist[NIALLOCID] = { 0 };

#define SZARENA 4096
#define CLUSTER_SIZE (SZARENA/sz)
#define ROUNDUP64(addr, align) ((addr + align - 1) & ~((unsigned long)align - 1))
#define NCLUST_SLAB 1
#define IBCOM_AALLOC_ID_MRCACHE 0

static inline void *aalloc(size_t sz, int id)
{
#if 1   /* debug */
    return malloc(sz);
#else
    char *p = free_list_front[id];
    if ((unsigned long) p & (SZARENA - 1)) {
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
            q = mmap(NULL, ROUNDUP64(SZARENA * NCLUST_SLAB, 4096), PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#if NCLUST_SLAB > 1
            arena_flist[id] = q + SZARENA;
            for (p = arena_flist[id]; p < q + (NCLUST_SLAB - 1) * SZARENA; p += SZARENA) {
                ((free_list_t *) p)->next = p + SZARENA;
            }
            ((free_list_t *) p)->next = 0;
#endif
        }
        *((int *) q) = CLUSTER_SIZE - 1;
        //      dprintf("q=%llx\n", q);
        q += sz + (SZARENA % sz);
        free_list_front[id] = q + sz;
        return q;
    }
#endif
}

static inline void afree(const void *p, int id)
{
#if 1   /* debug */
    return free((void *) p);
#else
    p = (void *) ((unsigned long) p & ~(SZARENA - 1));
    if (!(--(*((int *) p)))) {
        ((free_list_t *) p)->next = arena_flist[id];
        arena_flist[id] = (char *) p;
    }
#endif
}

struct ibcom_reg_mr_listnode_t {
    struct ibcom_reg_mr_listnode_t *lru_next;
    struct ibcom_reg_mr_listnode_t *lru_prev;
};

struct ibcom_reg_mr_cache_entry_t {
    /* : public ibcom_reg_mr_listnode_t */
    struct ibcom_reg_mr_listnode_t *lru_next;
    struct ibcom_reg_mr_listnode_t *lru_prev;

    struct ibv_mr *mr;
    void *addr;
    int len;
    int refc;
};

static struct ibcom_reg_mr_listnode_t ibcom_reg_mr_cache[IBCOM_REG_MR_NLINE];

__inline__ int ibcom_hash_func(char *addr)
{
    unsigned int v = (unsigned int) (unsigned long) addr;
    //v = v >> IBCOM_REG_MR_LOGSZPAGE; /* assume it is page aligned */
    v = v & (IBCOM_REG_MR_NLINE - 1);
    return (int) v;
}

void ibcom_reg_mr_insert(struct ibcom_reg_mr_listnode_t *c, struct ibcom_reg_mr_listnode_t *e)
{
    struct ibcom_reg_mr_listnode_t *next;
    struct ibcom_reg_mr_listnode_t *prev;
    prev = c;
    next = prev->lru_next;
    e->lru_next = next;
    e->lru_prev = prev;
    next->lru_prev = e;
    prev->lru_next = e;
}

void ibcom_reg_mr_unlink(struct ibcom_reg_mr_listnode_t *e)
{
    struct ibcom_reg_mr_listnode_t *next, *prev;
    next = e->lru_next;
    prev = e->lru_prev;
    next->lru_prev = prev;
    prev->lru_next = next;
}

static inline void __lru_queue_display()
{
    struct ibcom_reg_mr_cache_entry_t *p;
    int i = 0;
    for (i = 0; i < IBCOM_REG_MR_NLINE; i++) {
        dprintf("---- hash %d\n", i);
        for (p = (struct ibcom_reg_mr_cache_entry_t *) ibcom_reg_mr_cache[i].lru_next;
             p != (struct ibcom_reg_mr_cache_entry_t *) &ibcom_reg_mr_cache[i];
             p = (struct ibcom_reg_mr_cache_entry_t *) p->lru_next) {
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

struct ibv_mr *ibcom_reg_mr_fetch(void *addr, int len)
{
#if 0   /* debug */
    struct ibv_mr *mr;
    int ibcom_errno = ibcom_reg_mr(addr, len, &mr);
    printf("mrcache,ibcom_reg_mr,error,addr=%p,len=%d,lkey=%08x,rkey=%08x\n", addr, len, mr->lkey,
           mr->rkey);
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
    struct ibcom_reg_mr_cache_entry_t *e;

#if 1   /*def HAVE_LIBDCFA */
    /* we can't change addr because ibv_post_send assumes mr->host_addr (output of this function)
     * must have an exact mirror value of addr (input of this function) */
    void *addr_aligned = addr;
    int len_aligned = len;
#else
    void *addr_aligned = (void *) ((unsigned long) addr & ~(IBCOM_REG_MR_SZPAGE - 1));
    int len_aligned =
        ((((unsigned long) addr + len) - (unsigned long) addr_aligned + IBCOM_REG_MR_SZPAGE -
          1) & ~(IBCOM_REG_MR_SZPAGE - 1));
#endif
    key = ibcom_hash_func(addr);

    dprintf("[MrCache] addr=%p, len=%d\n", addr, len);
    dprintf("[MrCache] aligned addr=%p, len=%d\n", addr_aligned, len_aligned);

    //__lru_queue_display();
    int way = 0;
    for (e = (struct ibcom_reg_mr_cache_entry_t *) ibcom_reg_mr_cache[key].lru_next;
         e != (struct ibcom_reg_mr_cache_entry_t *) &ibcom_reg_mr_cache[key];
         e = (struct ibcom_reg_mr_cache_entry_t *) e->lru_next, way++) {
        //dprintf("e=%p, e->hash_next=%p\n", e, e->lru_next);

        if (e->addr <= addr_aligned && addr_aligned + len_aligned <= e->addr + e->len) {
            dprintf
                ("ibcom_reg_mr_fetch,hit,entry addr=%p,len=%d,mr addr=%p,len=%ld,requested addr=%p,len=%d\n",
                 e->addr, e->len, e->mr->addr, e->mr->length, addr, len);
            goto hit;
        }
    }

    // miss

    // evict an entry and de-register its MR when the cache-set is full
    if (way > IBCOM_REG_MR_NWAY) {
        struct ibcom_reg_mr_cache_entry_t *victim =
            (struct ibcom_reg_mr_cache_entry_t *) e->lru_prev;
        ibcom_reg_mr_unlink((struct ibcom_reg_mr_listnode_t *) victim);

        dprintf("ibcom_reg_mr,evict,entry addr=%p,len=%d,mr addr=%p,len=%ld\n", e->addr, e->len,
                e->mr->addr, e->mr->length);
        int ibcom_errno = ibcom_dereg_mr(victim->mr);
        if (ibcom_errno) {
            printf("mrcache,ibcom_dereg_mr\n");
            goto fn_fail;
        }
        afree(victim, IBCOM_AALLOC_ID_MRCACHE);
    }

    e = aalloc(sizeof(struct ibcom_reg_mr_cache_entry_t), IBCOM_AALLOC_ID_MRCACHE);
    /* reference counter is used when evicting entry */
    e->refc = 1;

    dprintf("ibcom_reg_mr_fetch,miss,addr=%p,len=%d\n", addr_aligned, len_aligned);
    /* register memory */
    ibcom_errno = ibcom_reg_mr(addr_aligned, len_aligned, &e->mr);
    if (ibcom_errno != 0) {
        fprintf(stderr, "mrcache,ibcom_reg_mr\n");
        goto fn_fail;
    }
    e->addr = addr_aligned;
    e->len = len_aligned;

    dprintf("ibcom_reg_mr_fetch,fill,e=%p,key=%d,mr=%p,mr addr=%p,len=%ld,lkey=%08x,rkey=%08x\n", e,
            key, e->mr, e->mr->addr, e->mr->length, e->mr->lkey, e->mr->rkey);

    /* register to cache */
    ibcom_reg_mr_insert(&ibcom_reg_mr_cache[key], (struct ibcom_reg_mr_listnode_t *) e);

    //__lru_queue_display();

    goto fn_exit;

  hit:

    /* reference counter is used when evicting entry */
    e->refc++;
#if 0   /* disable for debug */
    /* move to head of the list */
    if (e != (struct ibcom_reg_mr_cache_entry_t *) ibcom_reg_mr_cache[key].lru_next) {
        ibcom_reg_mr_unlink((struct ibcom_reg_mr_listnode_t *) e);
        ibcom_reg_mr_insert(&ibcom_reg_mr_cache[key], (struct ibcom_reg_mr_listnode_t *) e);
    }
#endif
    dprintf("[MrCache] reuse e=%p,key=%d,mr=%p,refc=%d,addr=%p,len=%ld,lkey=%08x,rkey=%08x\n", e,
            key, e->mr, e->refc, e->mr->addr, e->mr->length, e->mr->lkey, e->mr->rkey);

    //__lru_queue_display();

  fn_exit:
    return e->mr;
  fn_fail:
    goto fn_exit;
#endif
}

void ibcom_reg_mr_dereg(struct ibv_mr *mr)
{

    struct ibcom_reg_mr_cache_entry_t *e;
    struct ibcom_reg_mr_cache_entry_t *zero = 0;
    unsigned long offset = (unsigned long) zero->mr;
    e = (struct ibcom_reg_mr_cache_entry_t *) ((unsigned long) mr - offset);
    e->refc--;

    dprintf("ibcom_reg_mr_dereg,entry=%p,mr=%p,addr=%p,refc=%d,offset=%lx\n", e, mr, e->mr->addr,
            e->refc, offset);
}

void ibcom_RegisterCacheInit()
{
    int i;

    /* Using the address to the start node to express the end of the list
     * instead of using NULL */
    for (i = 0; i < IBCOM_REG_MR_NLINE; i++) {
        ibcom_reg_mr_cache[i].lru_next = (struct ibcom_reg_mr_listnode_t *) &ibcom_reg_mr_cache[i];
        ibcom_reg_mr_cache[i].lru_prev = (struct ibcom_reg_mr_listnode_t *) &ibcom_reg_mr_cache[i];
    }

    dprintf("[MrCache] cache initializes %d entries\n", IBCOM_REG_MR_NLINE);
}

void ibcom_RegisterCacheDestroy()
{
    struct ibcom_reg_mr_cache_entry_t *p;
    int i = 0, cnt = 0;

    for (i = 0; i < IBCOM_REG_MR_NLINE; i++) {
        for (p = (struct ibcom_reg_mr_cache_entry_t *) ibcom_reg_mr_cache[i].lru_next;
             p != (struct ibcom_reg_mr_cache_entry_t *) &ibcom_reg_mr_cache[i];
             p = (struct ibcom_reg_mr_cache_entry_t *) p->lru_next) {
            if (p && p->addr > 0) {
                ibcom_dereg_mr(p->mr);
                afree(p, IBCOM_AALLOC_ID_MRCACHE);
                cnt++;
            }
        }
    }

    //__lru_queue_display();

    dprintf("[MrCache] cache destroyed %d entries\n", cnt);
}
