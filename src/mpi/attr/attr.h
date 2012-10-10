/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Portions of this code were written by Microsoft. Those portions are
 * Copyright (c) 2007 Microsoft Corporation. Microsoft grants
 * permission to use, reproduce, prepare derivative works, and to
 * redistribute to others. The code is licensed "as is." The User
 * bears the risk of using it. Microsoft gives no express warranties,
 * guarantees or conditions. To the extent permitted by law, Microsoft
 * excludes the implied warranties of merchantability, fitness for a
 * particular purpose and non-infringement.
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

