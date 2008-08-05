#include "adio.h"

typedef struct {
    ADIO_Offset offset;
    int proc;
    ADIO_Offset reg_max_len;
} heap_node_t;

typedef struct {
    heap_node_t *nodes;
    int size;
} heap_t;

/*static inline int parent(heap_t *heap, int i);
static inline int left(heap_t *heap, int i);
static inline int right(heap_t *heap, int i); */
void heapify(heap_t *heap, int i);
void print_heap(heap_t *heap);
void free_heap(heap_t *heap);
int create_heap(heap_t *heap, int size);
void build_heap(heap_t *heap);
void heap_insert(heap_t *heap, ADIO_Offset offset, int proc,
		 ADIO_Offset reg_max_len);
void heap_extract_min(heap_t *heap, ADIO_Offset* key, int *proc,
		      ADIO_Offset *reg_max_len);
