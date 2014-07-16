#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>

//#define __DEBUG__

#ifdef __DEBUG__
#define dprintf printf

#define NUM_USED(addr, align, size) \
    (((size_t)addr & ~((size_t)align - 1)) == (size_t)addr) ? \
        ((size_t)align / size) : \
        (((size_t)addr & (align - 1)) / size)

#else
#define dprintf(...)
#endif

static void _local_malloc_initialize_hook(void);
void *malloc(size_t size);
void free(void *addr);
void *realloc(void *addr, size_t size);
void *calloc(size_t nmemb, size_t size);

void (*__malloc_initialize_hook) (void) = _local_malloc_initialize_hook;

static pthread_mutex_t mutex;
static int __initialized_malloc = 0;
static int __tunnel_munmap = 0;

#define POOL_MIN_POW (5)
#define POOL_MAX_POW (14)
#define PAGE_SIZE (1UL << 12)

#define MMAPED_OFFSET_POW (8)
#define MMAPED_OFFSET (1UL << MMAPED_OFFSET_POW)        // 256byte

#define ARRAY_SIZE (64) // x86_64

#define DEFAULT_POOL_SIZE (1UL << 17)   // 128Kbyte
#define POOL_ALIGN_SIZE (DEFAULT_POOL_SIZE)

#define do_segfault  (*(unsigned int*)0 = 0)    // segmentation fault

struct free_list {
    struct free_list *next;
    struct free_list *prev;
};

#define CHUNK (sizeof(struct free_list))

static inline void list_init(struct free_list *head)
{
    head->next = head;
    head->prev = head;
}

static inline void __list_add(struct free_list *new, struct free_list *prev, struct free_list *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void __list_del(struct free_list *prev, struct free_list *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_add_head(struct free_list *new, struct free_list *head)
{
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct free_list *new, struct free_list *head)
{
    __list_add(new, head->prev, head);
}

static inline void list_del(struct free_list *list)
{
    __list_del(list->prev, list->next);

    list->prev = NULL;
    list->next = NULL;
}

static inline int is_list_empty(struct free_list *list)
{
    return (list->next == list) ? 1 : 0;
}

static struct free_list arena_flist[ARRAY_SIZE];

struct pool_info {
    struct free_list list;      /* 16byte (x86_64) */
    char *next_pos;             /*  8byte (x86_64) */
    uint16_t size;              /*  2byte */
    uint16_t num;               /*  2byte */
    uint16_t free_num;          /*  2byte */
    uint16_t pow;               /*  2byte */
    uint16_t hole_num;          /*  2byte */
    uint16_t num_per_page;      /*  2byte */
    uint16_t count;             /*  2byte */
};                              /* size of 'struct pool_info' must be smaller than MMAPED_OFFSET */

#ifdef __x86_64__
#define builtin_clz  __builtin_clzl
#define builtin_ctz  __builtin_ctzl
#else
#define builtin_clz  __builtin_clz
#define builtin_ctz  __builtin_ctz
#endif

/* Get a power of the argument */
static int powoftwo(size_t val)
{
    if (val <= (size_t) (1UL << POOL_MIN_POW))
        return POOL_MIN_POW;

    int shift_max;
#if defined(__x86_64__)
    shift_max = 64;
#else
    shift_max = 32;
#endif

    /* If 'val' is power-of-two, we use 'ctz' */

    return (val & (val - 1UL)) ? (shift_max - builtin_clz(val)) : builtin_ctz(val);
}

static void *__alloc_mmap(size_t size, size_t align)
{
    char *unaligned, *aligned;
    size_t misaligned;
    int ret;

    unaligned = mmap(0, size + align, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (unaligned == MAP_FAILED)
        return NULL;

    misaligned = (size_t) unaligned & ((size_t) align - 1);
    if (misaligned > 0) {
        size_t offset = align - misaligned;
        aligned = unaligned + offset;

        /* munmap : head */
        __tunnel_munmap = 1;
        ret = munmap(unaligned, offset);
        __tunnel_munmap = 0;
        if (ret)
            do_segfault;
    }
    else {
        aligned = unaligned;
        misaligned = align;
    }

    /* munmap : tail */
    __tunnel_munmap = 1;
    ret = munmap(aligned + size, misaligned);
    __tunnel_munmap = 0;
    if (ret)
        do_segfault;

    return (void *) aligned;
}

static void __init_pool_header_with_hole(struct pool_info *info, int i, int size)
{
    info->size = 1 << i;
    info->hole_num = (MMAPED_OFFSET >> i) + 1;
    info->num_per_page = PAGE_SIZE >> i;
    info->num = size >> i;
    info->free_num = info->hole_num;
    info->count = info->hole_num;
    info->pow = i;
    info->next_pos = (char *) info + info->size * info->hole_num;
}

static void __init_pool_header(struct pool_info *info, int i, int size)
{
    info->size = 1 << i;
    info->num = size >> i;
    info->free_num = 1;
    info->pow = i;
    info->next_pos = (char *) info + info->size;
}

static void _local_malloc_initialize_hook(void)
{
    int i, j;
    char *aligned;
    size_t size;
    int count;

    pthread_mutex_init(&mutex, NULL);

    pthread_mutex_lock(&mutex);
    __initialized_malloc = 1;

    for (i = 0; i < ARRAY_SIZE; i++) {
        /* init list */
        list_init(&arena_flist[i]);
    }

    /* Allocate initial mempool
     *
     * We do not use 2^0, ..., 2^(POOL_MIN_POW - 1) byte.
     */

    /* First, allocate a initial area by one-time mmap() and split it */
    count = POOL_MAX_POW - POOL_MIN_POW + 1;
    size = (size_t) DEFAULT_POOL_SIZE;  // default pool size is 128k

    aligned = (char *) __alloc_mmap(size * count, POOL_ALIGN_SIZE);

    if (aligned == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    /* split allcated area */
    for (i = POOL_MIN_POW; i < POOL_MIN_POW + count; i++) {
        struct pool_info *info;

        info = (struct pool_info *) aligned;

        if (i <= MMAPED_OFFSET_POW) {
            __init_pool_header_with_hole(info, i, size);

            int elem = (DEFAULT_POOL_SIZE - (info->hole_num * info->size)) / (CHUNK + info->size);
            struct free_list *block_head = (struct free_list *) info->next_pos;
            for (j = 0; j < elem; j++) {
                if (((size_t) ((char *) block_head + CHUNK) & ((size_t) PAGE_SIZE - 1)) !=
                    MMAPED_OFFSET) {
                    list_add_tail(block_head, &arena_flist[i]);
                }
                block_head = (struct free_list *) ((char *) block_head + CHUNK + info->size);
            }
        }
        else {
            __init_pool_header(info, i, size);
            /* add list tail */
            list_add_tail(&(info->list), &arena_flist[i]);
        }

        aligned += size;
    }

    pthread_mutex_unlock(&mutex);
}

void *malloc(size_t size)
{
    int i;
    int pow;
    char *ptr = NULL;

    if (!__initialized_malloc && __malloc_initialize_hook)
        __malloc_initialize_hook();

    pthread_mutex_lock(&mutex);

    pow = powoftwo(size);

    if (pow < 0 || pow >= ARRAY_SIZE)
        return NULL;

    if (is_list_empty(&arena_flist[pow])) {
        char *tmp;

        if (pow > POOL_MAX_POW) {
            /* create memory area by mmap */

            tmp = (char *) __alloc_mmap(((size_t) 1 << pow) + PAGE_SIZE, PAGE_SIZE);

            if (tmp == NULL) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }

            *(int *) tmp = pow; //store 'power' for free()

            ptr = (char *) tmp + MMAPED_OFFSET;

            dprintf("malloc(%lu) [2^%d] ==> CREATE mmaped %p\n", size, pow, ptr);
        }
        else {
            /* create new pool */
            struct pool_info *info;
            size_t alloc_sz = DEFAULT_POOL_SIZE;

            tmp = (char *) __alloc_mmap(alloc_sz, POOL_ALIGN_SIZE);

            if (tmp == NULL) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }

            info = (struct pool_info *) tmp;

            if (pow <= MMAPED_OFFSET_POW) {
                __init_pool_header_with_hole(info, pow, alloc_sz);

                int elem =
                    (DEFAULT_POOL_SIZE - (info->hole_num * info->size)) / (CHUNK + info->size);
                struct free_list *block_head = (struct free_list *) info->next_pos;
                for (i = 0; i < elem; i++) {
                    if (((size_t) ((char *) block_head + CHUNK) & ((size_t) PAGE_SIZE - 1)) !=
                        MMAPED_OFFSET) {
                        list_add_tail(block_head, &arena_flist[pow]);
                    }
                    block_head = (struct free_list *) ((char *) block_head + CHUNK + info->size);
                }

                /* use head elem */
                struct free_list *head = (struct free_list *) (arena_flist[pow].next);
                ptr = (char *) head + CHUNK;
                dprintf("malloc(%lu) [2^%d] ==> USE pool %p\n", size, pow, ptr);
                list_del(head);
            }
            else {
                __init_pool_header(info, pow, alloc_sz);
                list_add_tail(&(info->list), &arena_flist[pow]);

                ptr = info->next_pos;
                info->next_pos += info->size;

                if (pow <= MMAPED_OFFSET_POW)
                    info->count++;

                dprintf("malloc(%lu) [2^%d] ==> CREATE pool %p   use = %lu\n", size, pow, ptr,
                        NUM_USED(info->next_pos, POOL_ALIGN_SIZE, info->size));
            }
        }
    }
    else {
        if (pow > POOL_MAX_POW) {
            char *head = (char *) arena_flist[pow].next;

            list_del((struct free_list *) head);

            *(int *) head = pow;        //store 'power' for free()
            ptr = (char *) head + MMAPED_OFFSET;

            dprintf("malloc(%lu) [2^%d] ==> USE mmaped %p\n", size, pow, ptr);
        }
        else if (pow > MMAPED_OFFSET_POW) {
            struct pool_info *info = (struct pool_info *) (arena_flist[pow].next);

            ptr = info->next_pos;
            info->next_pos += info->size;

            dprintf("malloc(%lu) [2^%d] ==> USE pool %p   use = %lu\n", size, pow, ptr,
                    NUM_USED(info->next_pos, POOL_ALIGN_SIZE, info->size));

            /* if 'info->nex_pos' is aligned, all blocks are used */
            if (((size_t) info->next_pos & ~(POOL_ALIGN_SIZE - 1)) == (size_t) info->next_pos) {
                list_del(&(info->list));
            }
        }
        else {
            char *info = (char *) (arena_flist[pow].next);
            ptr = (char *) info + CHUNK;
            dprintf("malloc(%lu) [2^%d] ==> USE pool %p\n", size, pow, ptr);
            list_del((struct free_list *) info);
        }
    }

    pthread_mutex_unlock(&mutex);

    return ptr;
}

static inline void free_core(void *addr)
{
    pthread_mutex_lock(&mutex);

    if (((size_t) addr & ((size_t) PAGE_SIZE - 1)) == MMAPED_OFFSET) {
        char *head = (char *) addr - MMAPED_OFFSET;
        int power = (int) *(int *) head;

        dprintf("free(%p) --> free MMAPED [2^%d]\n", addr, power);
        list_add_tail((struct free_list *) head, &arena_flist[power]);
    }
    else {
        struct pool_info *info =
            (struct pool_info *) ((size_t) addr & ~((size_t) POOL_ALIGN_SIZE - 1));

        if (info->pow <= MMAPED_OFFSET_POW) {
            struct free_list *block_head = (struct free_list *) ((size_t) addr - CHUNK);
            list_add_head(block_head, &arena_flist[info->pow]);
            dprintf("free(%p) --> free BLOCK [2^%d]\n", addr, info->pow);
        }
        else {
            dprintf("free(%p) --> free POOL [2^%d] %lu / %u / %u (use / free / max)\n",
                    addr, info->pow,
                    NUM_USED(info->next_pos, POOL_ALIGN_SIZE, info->size),
                    info->free_num + 1, info->num);

            info->free_num++;
            if (info->free_num == info->num) {
                /* intialize for reuse */
                info->free_num = 1;
                info->next_pos = (char *) info + info->size;

                list_add_tail(&(info->list), &arena_flist[info->pow]);

                dprintf("       POOL [2^%d]   ALL FREED -> add list [%p]\n", info->pow,
                        &arena_flist[info->pow]);
            }
        }
    }

    pthread_mutex_unlock(&mutex);
}

void free(void *addr)
{
    if (addr) {
        free_core(addr);
        addr = NULL;
    }
}

void *realloc(void *addr, size_t size)
{
    void *tmp;

    dprintf("realloc(%p, %lu)\n", addr, size);

    tmp = malloc(size);

    if (addr != NULL) {
        int old_pow, new_pow, power;

        new_pow = powoftwo(size);

        /* get power of 'addr' area */
        if (((size_t) addr & ((size_t) PAGE_SIZE - 1)) == MMAPED_OFFSET) {
            char *head = (char *) addr - MMAPED_OFFSET;
            old_pow = (int) *(int *) head;
        }
        else {
            struct pool_info *info =
                (struct pool_info *) ((size_t) addr & ~((size_t) POOL_ALIGN_SIZE - 1));
            old_pow = info->pow;
        }

        if (old_pow < new_pow)
            power = old_pow;    /* expand */
        else
            power = new_pow;    /* shrink */

        memcpy((char *) tmp, (char *) addr, (size_t) 1 << power);

        free_core(addr);
    }

    addr = tmp;

    return tmp;
}

void *calloc(size_t nmemb, size_t size)
{
    size_t total_sz;
    char *ptr;

    if (!nmemb || !size)
        return NULL;

    total_sz = nmemb * size;
    ptr = malloc(total_sz);
    if (ptr == NULL)
        return NULL;

    memset(ptr, 0, total_sz);

    return ptr;
}

int munmap(void *addr, size_t length)
{
    if (__tunnel_munmap) {
        dprintf("munmap(%p, 0x%lx)\n", addr, length);

        return syscall(__NR_munmap, addr, length);
    }
    else {
        /* do nothing */
    }

    return 0;
}
