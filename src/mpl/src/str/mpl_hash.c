/*
*  Copyright (C) by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

/** Rationale:
 *    An open addressing hash implementation specialized for string keys and string
 *    values. All strings are stored in a managed string pool in a insertion only
 *    manner. Deletion is achieved by simply overwrite the hash item with sentinel
 *    value/NULL. Deletion and reassignement will result in expired strings still
 *    being held in memory, which may be a concern if the application requires
 *    frequent deletion and modification and memory consumption is of concern. All
 *    memory will be released upon final free.
 *
 *    Compared to uthash.h, the usage interface is cleaner and more intuitive --
 *    hash_new, hash_set, hash_get, hash_has, and hash_free. In comparison, uthash
 *    requires extra data structure, does not manage string memory, and the code is
 *    more complex. This implementation is faster than uthash as well.
 */

#include "mpl.h"
#include "mpl_hash.h"

#define STRPOOL_STR(strpool, i) \
    (char *) ((strpool)->pc_pool + (strpool)->pn_str[i])

#define STRPOOL_STRLEN(strpool, i) \
    ((strpool)->pn_str[(i)+1] - (strpool)->pn_str[i] - 1)

static int f_strhash_lookup(void *hash, unsigned char *key, int keylen);
static int f_strhash_lookup_left(void *hash, unsigned char *key, int keylen);
static int f_addto_strpool(struct strpool *p_strpool, const char *s, int n);
static void f_strhash_resize(void *hash, int n_size);
static void f_resize_strpool_n(struct strpool *p_strpool, int n_size, int n_avg);

struct MPL_hash *MPL_hash_new(void)
{
    struct MPL_hash *hv;

    hv = (struct MPL_hash *) calloc(1, sizeof(struct MPL_hash));
    return hv;
}

int MPL_hash_has(struct MPL_hash *hv, const char *s_key)
{
    int k;

    k = f_strhash_lookup(hv, (unsigned char *) s_key, strlen(s_key));
    return hv->p_exist[k];
}

void MPL_hash_set(struct MPL_hash *hv, const char *s_key, const char *s_value)
{
    int k;

    k = f_strhash_lookup_left(hv, (unsigned char *) s_key, strlen(s_key));
    hv->p_val[k] = f_addto_strpool(&hv->pool, s_value, strlen(s_value));
}

char *MPL_hash_get(struct MPL_hash *hv, const char *s_key)
{
    int k;
    struct strpool *p_strpool;

    k = f_strhash_lookup(hv, (unsigned char *) s_key, strlen(s_key));
    if (hv->p_exist[k]) {
        p_strpool = &hv->pool;
        return STRPOOL_STR(p_strpool, hv->p_val[k]);
    } else {
        return NULL;
    }
}

void MPL_hash_free(struct MPL_hash *hv)
{
    MPL_free(hv->p_key);
    MPL_free(hv->p_exist);
    MPL_free(hv->p_val);
    MPL_free(hv->pool.pn_str);
    MPL_free(hv->pool.pc_pool);
    MPL_free(hv);
}

int f_strhash_lookup(void *hash, unsigned char *key, int keylen)
{
    struct MPL_hash *h = hash;
    unsigned int tu_h;
    int k;
    struct strpool *p_strpool;

    if (keylen == 0) {
        keylen = strlen((char *) key);
    }
    if (h->n_size == 0) {
        return 0;
    }
    tu_h = 2166136261u;
    for (int i = 0; i < keylen; i++) {
        tu_h = tu_h ^ key[i];
        tu_h = tu_h * 16777619;
    }
    k = (int) (tu_h % h->n_size);
    p_strpool = &h->pool;

    while (1) {
        if (!h->p_exist[k]) {
            return k;
        } else {
            if ((STRPOOL_STRLEN(p_strpool, h->p_key[k]) == keylen) &&
                (strncmp(STRPOOL_STR(p_strpool, h->p_key[k]), (char *) key, keylen) == 0)) {
                return k;
            }
        }

        if (k == 0) {
            k = h->n_size - 1;
        } else {
            k--;
        }
    }
}

int f_strhash_lookup_left(void *hash, unsigned char *key, int keylen)
{
    struct MPL_hash *h = hash;
    int k;

    if (keylen == 0) {
        keylen = strlen((char *) key);
    }
    if (h->n_size == 0 || h->n_count + 1 >= h->n_size ||
        (h->n_size > 20 && h->n_count >= h->n_size * 85 / 100)) {
        f_strhash_resize(h, 0);
    }
    k = f_strhash_lookup(h, key, keylen);
    if (!h->p_exist[k]) {
        h->p_key[k] = f_addto_strpool(&h->pool, (char *) key, keylen);
        h->p_exist[k] = 1;
        h->n_count++;
    }
    return k;
}

int f_addto_strpool(struct strpool *p_strpool, const char *s, int n)
{
    int n_size;
    int tn_avg_str_size;

    if (p_strpool->i_str + 2 >= p_strpool->n_str) {
        n_size = p_strpool->i_str * 3 / 2 + 10;
        f_resize_strpool_n(p_strpool, n_size, 0);
    }

    if (n == 0) {
        n = strlen(s);
    }
    if (p_strpool->i_pool + n >= p_strpool->pool_size) {
        tn_avg_str_size = 6;
        if (p_strpool->i_str > 0) {
            tn_avg_str_size = p_strpool->i_pool / p_strpool->i_str + 1;
        }
        p_strpool->pool_size = tn_avg_str_size * p_strpool->n_str + n;
        p_strpool->pc_pool = realloc(p_strpool->pc_pool, p_strpool->pool_size);
    }
    memcpy(p_strpool->pc_pool + p_strpool->i_pool, s, n);
    p_strpool->i_str++;
    p_strpool->i_pool += n;
    assert(p_strpool->i_str > 0);
    assert(p_strpool->i_pool > 0);
    p_strpool->pc_pool[p_strpool->i_pool] = '\0';
    p_strpool->i_pool += 1;
    p_strpool->pn_str[p_strpool->i_str] = p_strpool->i_pool;

    return p_strpool->i_str - 1;
}

void f_strhash_resize(void *hash, int n_size)
{
    struct MPL_hash *h = hash;
    char *tp_exist = h->p_exist;
    int *tp_key = h->p_key;
    int tn_old_size;
    int *tp_val = h->p_val;
    struct strpool *p_strpool;
    int k;

    if (n_size == 0) {
        if (h->n_size <= 0) {
            n_size = 10;
        } else {
            n_size = h->n_size * 5 / 3;
        }
    } else if (n_size <= h->n_size) {
        return;
    }

    tn_old_size = h->n_size;

    h->n_size = n_size;
    h->p_key = (int *) MPL_calloc(n_size, sizeof(int), MPL_MEM_OTHER);
    h->p_exist = (char *) MPL_calloc(n_size, sizeof(char), MPL_MEM_OTHER);
    h->p_val = (void *) MPL_calloc(n_size, sizeof(int), MPL_MEM_OTHER);

    p_strpool = &h->pool;
    if (tn_old_size > 0) {
        for (int i = 0; i < tn_old_size; i++) {
            if (tp_exist[i]) {

                k = f_strhash_lookup(h, (unsigned char *) STRPOOL_STR(p_strpool, tp_key[i]),
                                     STRPOOL_STRLEN(p_strpool, tp_key[i]));
                h->p_exist[k] = 1;
                h->p_key[k] = tp_key[i];
                h->p_val[k] = tp_val[i];
            }
        }
        MPL_free(tp_exist);
        MPL_free(tp_key);
        MPL_free(tp_val);
    }
}

void f_resize_strpool_n(struct strpool *p_strpool, int n_size, int n_avg)
{
    if (n_size <= p_strpool->n_str) {
        return;
    }
    p_strpool->n_str = n_size;
    p_strpool->pn_str =
        MPL_realloc(p_strpool->pn_str, p_strpool->n_str * sizeof(int), MPL_MEM_OTHER);

    if (n_avg == 0) {
        n_avg = 6;
        if (p_strpool->i_str > 0) {
            n_avg = p_strpool->i_pool / p_strpool->i_str + 1;
        }
    }
    if (n_avg * p_strpool->n_str > p_strpool->pool_size) {
        p_strpool->pool_size = n_avg * p_strpool->n_str;
        p_strpool->pc_pool = MPL_realloc(p_strpool->pc_pool, p_strpool->pool_size, MPL_MEM_OTHER);
        p_strpool->pc_pool[p_strpool->pool_size - 1] = 0;
    }

    if (p_strpool->i_str == 0) {
        p_strpool->pn_str[0] = 0;
    }
}
