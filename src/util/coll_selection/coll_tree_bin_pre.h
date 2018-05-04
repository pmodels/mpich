#ifndef MPIU_COLL_SELECTION_TREE_PRE_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_PRE_H_INCLUDED

#include <stddef.h>

struct MPIU_COLL_SELECTION_tree_node;

#define MPIU_COLL_SELECTION_storage_handler ptrdiff_t
#define MPIU_COLL_SELECTION_HANDLER_TO_POINTER(_handler) ((struct MPIU_COLL_SELECTION_tree_node *)(MPIU_COLL_SELECTION_offset_tree + _handler))
#define MPIU_COLL_SELECTION_NODE_FIELD(_node, _field)  (MPIU_COLL_SELECTION_HANDLER_TO_POINTER(_node)->_field)

#define MPIU_COLL_SELECTION_NULL_ENTRY ((MPIU_COLL_SELECTION_storage_handler)-1)
#define MPIU_COLL_SELECTION_STORAGE_SIZE (1024*1024)

extern MPIU_COLL_SELECTION_storage_handler MPIU_COLL_SELECTION_tree_current_offset;
extern char MPIU_COLL_SELECTION_offset_tree[];

#endif /* MPIU_COLL_SELECTION_PRE_TYPES_H_INCLUDED */
