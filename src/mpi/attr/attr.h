/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
  Keyval and attribute storage
 */
extern MPIU_Object_alloc_t MPID_Keyval_mem;
extern MPIU_Object_alloc_t MPID_Attr_mem;
extern MPID_Keyval MPID_Keyval_direct[];

extern int MPIR_Attr_dup_list( int, MPID_Attribute *, MPID_Attribute ** );
extern int MPIR_Attr_delete_list( int, MPID_Attribute ** );
extern MPID_Attribute *MPID_Attr_alloc(void);
extern void MPID_Attr_free(MPID_Attribute *attr_ptr);
extern int MPIR_Call_attr_delete( int, MPID_Attribute * );
extern int MPIR_Call_attr_copy( int, MPID_Attribute *, void**, int* );

