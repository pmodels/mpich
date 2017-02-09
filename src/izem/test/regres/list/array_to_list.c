#include <stdlib.h>
#include <stdio.h>
#include <list/zm_sdlist.h>

#define TEST_NELEMTS 10000

int main (int argc, char** argv) {
    int rand_array[TEST_NELEMTS], i, errs = 0;
    void *data;
    for (i=0; i<TEST_NELEMTS; i++)
        rand_array[i] = rand();
    zm_sdlist_t list1, list2, list3;

    zm_sdlist_init(&list1);
    zm_sdlist_init(&list2);
    zm_sdlist_init(&list3);

    /* push the array's elements to list1 */
    for (i=0; i<TEST_NELEMTS; i++)
        zm_sdlist_push_back(&list1, &rand_array[i]);

    if(zm_sdlist_length(list1) != TEST_NELEMTS) {
        errs++;
        fprintf(stderr, "Error, length of list1 should be %d but got %d\n", TEST_NELEMTS, zm_sdlist_length(list1));
    }

    /* pop the elements from list1 and push them to list2 */
    data = zm_sdlist_pop_front(&list1);
    while (data != NULL) {
        zm_sdlist_push_back(&list2, data);
        data = zm_sdlist_pop_front(&list1);
    }

    if(zm_sdlist_length(list2) != TEST_NELEMTS) {
        errs++;
        fprintf(stderr, "Error, length of list2 should be %d but got %d\n", TEST_NELEMTS, zm_sdlist_length(list2));
    }

    /* iterate through list2, remove element, and push it to list3*/
    zm_sdlnode_t *cur_node = zm_sdlist_begin(list2);
    if(cur_node != NULL) {
        do {
           zm_sdlist_push_back(&list3, cur_node->data);
           if(cur_node == zm_sdlist_end(list2)) {
               zm_sdlist_remove(&list2, cur_node->data);
               break;
           }
           zm_sdlist_remove(&list2, cur_node->data);
           cur_node = cur_node->next;
       } while (1);
    }

    if(zm_sdlist_length(list3) != TEST_NELEMTS) {
        errs++;
        fprintf(stderr, "Error, length of list3 should be %d but got %d\n", TEST_NELEMTS, zm_sdlist_length(list3));
    }

    /* verify that list3 contains the same data as the array */
    i = 0;
    data = zm_sdlist_pop_front(&list3);
    while (data != NULL) {
        if(rand_array[i] != *(int*)data) {
            errs++;
            fprintf(stderr, "Error at element %d\n", i);
        }
        data = zm_sdlist_pop_front(&list3);
        i++;
    }
    if(errs > 0)    fprintf(stderr, "Failed with %d errors\n", errs);
    else            printf("Pass\n");

    return 0;
}
