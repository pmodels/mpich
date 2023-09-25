#ifndef MPL_HASH_H_INCLUDED
#define MPL_HASH_H_INCLUDED

struct strpool {
    int i_str;
    int i_pool;
    int n_str;
    size_t pool_size;
    int *pn_str;
    unsigned char *pc_pool;
};

struct MPL_hash {
    int n_size;
    int n_count;
    char *p_exist;
    int *p_key;
    struct strpool pool;
    int *p_val;
};

struct MPL_hash *MPL_hash_new(void);
int MPL_hash_has(struct MPL_hash *hv, const char *s_key);
void MPL_hash_set(struct MPL_hash *hv, const char *s_key, const char *s_value);
char *MPL_hash_get(struct MPL_hash *hv, const char *s_key);
void MPL_hash_free(struct MPL_hash *hv);

#endif /* MPL_HASH_H_INCLUDED */
