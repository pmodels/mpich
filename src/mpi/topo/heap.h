/*---------------------------------------------------------------------*/
/*     (C) Copyright 2017 Parallel Processing Research Laboratory      */
/*                   Queen's University at Kingston                    */
/*                Neighborhood Collective Communication                */
/*                    Seyed Hessamedin Mirsadeghi                      */
/*---------------------------------------------------------------------*/
typedef struct heap_element {
	int key;
	int value;
	int paired;
}heap_element;

typedef struct heap {
	int count;
	int arr_size;
	heap_element **heap_arr;
}heap;

int heap_init(heap *h, int arr_size);
int heap_insert(heap *h,  heap_element *e);
int heap_remove_max(heap *h);
int heap_remove_index(heap *h, int index);
int heap_find_value(heap *h, int value);
int heap_peek_max_key(heap *h);
int heap_peek_max_value(heap *h);
int heap_get_keys_array(heap *h, int *keys);
int heap_get_values_array(heap *h, int *values);
int heap_is_empty(heap *h);
int heap_peek_key_at_index(heap *h, int index);
int heap_peek_value_at_index(heap *h, int index);
int heap_free_array(heap *h);
