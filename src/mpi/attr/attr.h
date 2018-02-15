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
