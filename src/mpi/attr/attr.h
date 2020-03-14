/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef ATTR_H_INCLUDED
#define ATTR_H_INCLUDED

/*
  Keyval and attribute storage
 */
extern MPIR_Object_alloc_t MPII_Keyval_mem;
extern MPIR_Object_alloc_t MPID_Attr_mem;
extern MPII_Keyval MPII_Keyval_direct[];

extern int MPIR_Attr_dup_list(int, MPIR_Attribute *, MPIR_Attribute **);
extern int MPIR_Attr_delete_list(int, MPIR_Attribute **);
extern MPIR_Attribute *MPID_Attr_alloc(void);
extern void MPID_Attr_free(MPIR_Attribute * attr_ptr);
extern int MPIR_Call_attr_delete(int, MPIR_Attribute *);
extern int MPIR_Call_attr_copy(int, MPIR_Attribute *, void **, int *);

#endif /* ATTR_H_INCLUDED */
