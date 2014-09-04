/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Interfaces in this file are intended to be used by modules other than
 * MPIR_T. They use these interfaces to declare, create, operate and expose
 * enums, control variables, performance variables and categories.
 */
#ifndef MPIT_H_INCLUDED
#define MPIT_H_INCLUDED

#include "mpitimpl.h"

/* Stamp to indicate a change in categories */
extern int cat_stamp;

/* Tables to store enums, categories, cvars, pvars.
 * All tables need random access except enum_table. But we still implement
 * enum_table as an array, since it is handy and element size of
 * enum_table (i.e., sizeof(enum_item_t)) is small.
 */
extern UT_array *enum_table;
extern UT_array *cat_table;
extern UT_array *cvar_table;
extern UT_array *pvar_table;

/* Hash tables to quick locate category, cvar, pvar indices by their names */
extern name2index_hash_t *cat_hash;
extern name2index_hash_t *cvar_hash;
/* pvar names are unique per pvar class. So we use multiple hashs */
extern name2index_hash_t *pvar_hashs[MPIR_T_PVAR_CLASS_NUMBER];

/* See description in mpit.c */
extern void MPIR_T_enum_create(const char *name, MPI_T_enum *handle);
extern void MPIR_T_enum_add_item(MPI_T_enum handle, const char *item_name, int item_value);
extern int MPIR_T_cat_add_pvar(const char *cat_name, int pvar_index);
extern int MPIR_T_cat_add_cvar(const char *cat_name, int cvar_index);
extern int MPIR_T_cat_add_subcat(const char *parent_name, const char *child_name);
extern int MPIR_T_cat_add_desc(const char *cat_name, const char *cat_desc);

static inline cvar_table_entry_t * LOOKUP_CVAR_BY_NAME(const char* cvar_name)
{
    int cvar_idx;
    name2index_hash_t *hash_entry;
    HASH_FIND_STR(cvar_hash, cvar_name, hash_entry);
    MPIU_Assert(hash_entry != NULL);
    cvar_idx = hash_entry->idx;
    return (cvar_table_entry_t *)utarray_eltptr(cvar_table, cvar_idx);
}

/* Helper macros for getting the default value of a cvar */
#define MPIR_CVAR_GET_DEFAULT_INT(name_,out_ptr_)   \
    do {  \
        cvar_table_entry_t *cvar = LOOKUP_CVAR_BY_NAME(#name_); \
        *(out_ptr_) = cvar->defaultval.d; \
    } while (0)

#define MPIR_CVAR_GET_DEFAULT_BOOLEAN(name_,out_ptr_)   \
    do {  \
        MPIR_CVAR_GET_DEFAULT_INT(name_,out_ptr_); \
    } while (0)

#define MPIR_CVAR_GET_DEFAULT_UNSIGNED(name_,out_ptr_)   \
    do {  \
        cvar_table_entry_t *cvar = LOOKUP_CVAR_BY_NAME(#name_); \
        *(out_ptr_) = cvar->defaultval.u; \
    } while (0)

#define MPIR_CVAR_GET_DEFAULT_UNSIGNED_LONG(name_,out_ptr_)   \
    do {  \
        cvar_table_entry_t *cvar = LOOKUP_CVAR_BY_NAME(#name_); \
        *(out_ptr_) = cvar->defaultval.ul; \
    } while (0)

#define MPIR_CVAR_GET_DEFAULT_UNSIGNED_LONG_LONG(name_,out_ptr_)   \
    do {  \
        cvar_table_entry_t *cvar = LOOKUP_CVAR_BY_NAME(#name_); \
        *(out_ptr_) = cvar->defaultval.ull; \
    } while (0)

#define MPIR_CVAR_GET_DEFAULT_DOUBLE(name_,out_ptr_)   \
    do {  \
        cvar_table_entry_t *cvar = LOOKUP_CVAR_BY_NAME(#name_); \
        *(out_ptr_) = cvar->defaultval.f; \
    } while (0)

#define MPIR_CVAR_GET_DEFAULT_STRING(name_,out_ptr_)   \
    do {  \
        cvar_table_entry_t *cvar = LOOKUP_CVAR_BY_NAME(#name_); \
        *(out_ptr_) = cvar->defaultval.str; \
    } while (0)

#define MPIR_CVAR_GET_DEFAULT_RANGE(name_,out_ptr_)   \
    do {  \
        cvar_table_entry_t *cvar = LOOKUP_CVAR_BY_NAME(#name_); \
        *(out_ptr_) = cvar->defaultval.range; \
    } while (0)

/* Register a static cvar. A static pvar has NO binding and its address
 * and count are known at registeration time.
 * Attention: name_ is a token not a string
*/
#define MPIR_T_CVAR_REGISTER_STATIC(dtype_, name_, addr_, count_, verb_, \
            scope_, default_, cat_, desc_) \
    do { \
        MPIU_Assert(count_ > 0); \
        MPIR_T_CVAR_REGISTER_impl(dtype_, #name_, addr_, count_, MPI_T_ENUM_NULL, \
            verb_, MPI_T_BIND_NO_OBJECT, scope_, NULL, NULL, default_, cat_, desc_); \
    } while (0)

/* Register a dynamic cvar, which may have object binding and whose
 * address and count may be unknown at registeration time. It is
 * developers' duty to provide self-contained arguments.
*/
#define MPIR_T_CVAR_REGISTER_DYNAMIC(dtype_, name_, addr_, count_, etype_, \
            verb_, bind_, scope_, get_addr_, get_count_, default_, cat_, desc_) \
    do { \
        MPIU_Assert(addr_ != NULL || get_addr_ != NULL); \
        MPIU_Assert(count_ > 0 || get_count_ != NULL); \
        MPIR_T_CVAR_REGISTER_impl(dtype_, #name_, addr_, count_, etype_, \
            verb_, bind_, scope_, get_addr_, get_count_, default_, cat_, desc_); \
    } while (0)

/* stmt_ is executed only when ENABLE_PVAR_#MODULE is defined as 1 */
#define MPIR_T_PVAR_STMT(MODULE, stmt_) \
    PVAR_GATED_ACTION(MODULE, stmt_)

/* The following are interfaces for each pvar classe,
 * basically including delcaration, access and registeration.
 */

/* STATE */
#define MPIR_T_PVAR_INT_STATE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_INT_STATE_DECL_impl(name_))

#define MPIR_T_PVAR_INT_STATE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_INT_STATE_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_INT_STATE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_INT_STATE_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_STATE_SET_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_STATE_SET_VAR_impl(ptr_, val_))
/* Not gated by MODULE, since it is supposed to be a rvalue */
#define MPIR_T_PVAR_STATE_GET_VAR(ptr_) \
    MPIR_T_PVAR_STATE_GET_VAR_impl(ptr_)

#define MPIR_T_PVAR_STATE_SET(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_STATE_SET_impl(name_, val_))
#define MPIR_T_PVAR_STATE_GET(name_) \
    MPIR_T_PVAR_STATE_GET_impl(name_)

#define MPIR_T_PVAR_STATE_REGISTER_STATIC(MODULE, dtype_, name_, \
            initval_, etype_, verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_STATE_REGISTER_STATIC_impl(dtype_, name_, \
            initval_, etype_, verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_STATE_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, etype_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_STATE_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, etype_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_))

/* LEVEL */
#define MPIR_T_PVAR_UINT_LEVEL_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_LEVEL_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG_LEVEL_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_LEVEL_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG2_LEVEL_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_LEVEL_DECL_impl(name_))
#define MPIR_T_PVAR_DOUBLE_LEVEL_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_LEVEL_DECL_impl(name_))

#define MPIR_T_PVAR_UINT_LEVEL_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_LEVEL_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG_LEVEL_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_LEVEL_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG2_LEVEL_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_LEVEL_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_DOUBLE_LEVEL_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_LEVEL_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_UINT_LEVEL_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_LEVEL_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG_LEVEL_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_LEVEL_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG2_LEVEL_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_LEVEL_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_DOUBLE_LEVEL_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_LEVEL_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_LEVEL_SET_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_SET_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_LEVEL_GET_VAR(ptr_) \
    MPIR_T_PVAR_LEVEL_GET_VAR_impl(ptr_)
#define MPIR_T_PVAR_LEVEL_INC_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_INC_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_LEVEL_DEC_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_DEC_VAR_impl(ptr_, val_))

#define MPIR_T_PVAR_LEVEL_SET(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_SET_impl(name_, val_))
#define MPIR_T_PVAR_LEVEL_GET(name_) \
    MPIR_T_PVAR_LEVEL_GET_impl(name_)
#define MPIR_T_PVAR_LEVEL_INC(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_INC_impl(name_, val_))
#define MPIR_T_PVAR_LEVEL_DEC(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_DEC_impl(name_, val_))

#define MPIR_T_PVAR_LEVEL_REGISTER_STATIC(MODULE, dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_REGISTER_STATIC_impl(dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_LEVEL_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LEVEL_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count, cat_, desc_))

/* SIZE */
#define MPIR_T_PVAR_UINT_SIZE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_SIZE_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG_SIZE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_SIZE_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG2_SIZE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_SIZE_DECL_impl(name_))
#define MPIR_T_PVAR_DOUBLE_SIZE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_SIZE_DECL_impl(name_))

#define MPIR_T_PVAR_UINT_SIZE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_SIZE_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG_SIZE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_SIZE_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG2_SIZE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_SIZE_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_DOUBLE_SIZE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_SIZE_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_UINT_SIZE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_SIZE_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG_SIZE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_SIZE_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG2_SIZE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_SIZE_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_DOUBLE_SIZE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_SIZE_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_SIZE_SET_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_SIZE_SET_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_SIZE_GET_VAR(ptr_) \
    MPIR_T_PVAR_SIZE_GET_VAR_impl(ptr_)

#define MPIR_T_PVAR_SIZE_SET(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_SIZE_SET_impl(name_, val_))
#define MPIR_T_PVAR_SIZE_GET(name_) \
    MPIR_T_PVAR_SIZE_GET_impl(name_)

#define MPIR_T_PVAR_SIZE_REGISTER_STATIC(MODULE, dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_SIZE_REGISTER_STATIC_impl(dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_SIZE_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_SIZE_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count, cat_, desc_))

/* PERCENTAGE */
#define MPIR_T_PVAR_DOUBLE_PERCENTAGE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_PERCENTAGE_DECL_impl(name_))

#define MPIR_T_PVAR_DOUBLE_PERCENTAGE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_PERCENTAGE_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_DOUBLE_PERCENTAGE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_PERCENTAGE_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_PERCENTAGE_SET_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_PERCENTAGE_SET_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_PERCENTAGE_GET_VAR(MODULE, ptr_) \
    MPIR_T_PVAR_PERCENTAGE_GET_VAR_impl(ptr_)

#define MPIR_T_PVAR_PERCENTAGE_SET(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_PERCENTAGE_SET_impl(name_, val_))
#define MPIR_T_PVAR_PERCENTAGE_GET(MODULE, name_) \
    MPIR_T_PVAR_PERCENTAGE_GET_impl(name_)

#define MPIR_T_PVAR_PERCENTAGE_REGISTER_STATIC(MODULE, dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_PERCENTAGE_REGISTER_STATIC_impl(dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_PERCENTAGE_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_PERCENTAGE_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count, cat_, desc_))

/* COUNTER */
#define MPIR_T_PVAR_UINT_COUNTER_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_COUNTER_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG_COUNTER_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_COUNTER_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG2_COUNTER_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_COUNTER_DECL_impl(name_))

#define MPIR_T_PVAR_UINT_COUNTER_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_COUNTER_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG_COUNTER_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_COUNTER_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG2_COUNTER_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_COUNTER_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_UINT_COUNTER_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_COUNTER_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG_COUNTER_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_COUNTER_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG2_COUNTER_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_COUNTER_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_COUNTER_INIT_VAR(MODULE, ptr_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_INIT_VAR_impl(ptr_))
#define MPIR_T_PVAR_COUNTER_GET_VAR(ptr_) \
    MPIR_T_PVAR_COUNTER_GET_VAR_impl(ptr_)
#define MPIR_T_PVAR_COUNTER_INC_VAR(MODULE, ptr_, inc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_INC_VAR_impl(ptr_, inc_))

#define MPIR_T_PVAR_COUNTER_INIT(MODULE, name_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_INIT_impl(name_))
#define MPIR_T_PVAR_COUNTER_GET(name_) \
    MPIR_T_PVAR_COUNTER_GET_impl(name_)
#define MPIR_T_PVAR_COUNTER_INC(MODULE, name_, inc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_INC_impl(name_, inc_))
#define MPIR_T_PVAR_COUNTER_ADDR(name_) \
    MPIR_T_PVAR_COUNTER_ADDR_impl(name_)

#define MPIR_T_PVAR_COUNTER_REGISTER_STATIC(MODULE, dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_REGISTER_STATIC_impl(dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_COUNTER_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_))

/* COUNTER ARRAY for user's convenience */
#define MPIR_T_PVAR_UINT_COUNTER_ARRAY_DECL(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_COUNTER_ARRAY_DECL_impl(name_, len_))
#define MPIR_T_PVAR_ULONG_COUNTER_ARRAY_DECL(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_COUNTER_ARRAY_DECL_impl(name_, len_))
#define MPIR_T_PVAR_ULONG2_COUNTER_ARRAY_DECL(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_COUNTER_ARRAY_DECL_impl(name_, len_))

#define MPIR_T_PVAR_UINT_COUNTER_ARRAY_DECL_STATIC(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_COUNTER_ARRAY_DECL_STATIC_impl(name_, len_))
#define MPIR_T_PVAR_ULONG_COUNTER_ARRAY_DECL_STATIC(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_COUNTER_ARRAY_DECL_STATIC_impl(name_, len_))
#define MPIR_T_PVAR_ULONG2_COUNTER_ARRAY_DECL_STATIC(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_COUNTER_ARRAY_DECL_STATIC_impl(name_, len_))

#define MPIR_T_PVAR_UINT_COUNTER_ARRAY_DECL_EXTERN(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_COUNTER_ARRAY_DECL_EXTERN_impl(name_, len_))
#define MPIR_T_PVAR_ULONG_COUNTER_ARRAY_DECL_EXTERN(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_COUNTER_ARRAY_DECL_EXTERN_impl(name_, len_))
#define MPIR_T_PVAR_ULONG2_COUNTER_ARRAY_DECL_EXTERN(MODULE, name_, len_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_COUNTER_ARRAY_DECL_EXTERN_impl(name_, len_))

#define MPIR_T_PVAR_COUNTER_ARRAY_INIT_VAR(MODULE, ptr_, count_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_ARRAY_INIT_VAR_impl(ptr_, count_))
#define MPIR_T_PVAR_COUNTER_ARRAY_GET_VAR(ptr_, idx_) \
    MPIR_T_PVAR_COUNTER_ARRAY_GET_VAR_impl(ptr_, idx_)
#define MPIR_T_PVAR_COUNTER_ARRAY_INC_VAR(MODULE, ptr_, idx_, inc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_ARRAY_INC_VAR_impl(ptr_, idx_, inc_))

#define MPIR_T_PVAR_COUNTER_ARRAY_INIT(MODULE, name_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_ARRAY_INIT_impl(name_))
#define MPIR_T_PVAR_COUNTER_ARRAY_GET(name_, idx_) \
    MPIR_T_PVAR_COUNTER_ARRAY_GET_impl(name_, idx_)
#define MPIR_T_PVAR_COUNTER_ARRAY_INC(MODULE, ptr_, idx_, inc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_ARRAY_INC_impl(ptr_, idx_, inc_))

#define MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC(MODULE, dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_COUNTER_ARRAY_REGISTER_STATIC_impl(dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_))

/* ARRGEGATE */
#define MPIR_T_PVAR_UINT_AGGREGATE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_AGGREGATE_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG_AGGREGATE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_AGGREGATE_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG2_AGGREGATE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG2_AGGREGATE_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_impl(name_))

#define MPIR_T_PVAR_UINT_AGGREGATE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_AGGREGATE_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG_AGGREGATE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_AGGREGATE_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_UINT_AGGREGATE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_AGGREGATE_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG_AGGREGATE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_AGGREGATE_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_AGGREGATE_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_AGGREGATE_INIT_VAR(MODULE, ptr_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_AGGREGATE_INIT_VAR_impl(ptr_))
#define MPIR_T_PVAR_AGGREGATE_GET_VAR(ptr_) \
    MPIR_T_PVAR_AGGREGATE_GET_VAR_impl(ptr_)
#define MPIR_T_PVAR_AGGREGATE_INC_VAR(MODULE, ptr_, inc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_AGGREGATE_INC_VAR_impl(ptr_, inc_))

#define MPIR_T_PVAR_AGGREGATE_INIT(MODULE, name_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_AGGREGATE_INIT_impl(name_))
#define MPIR_T_PVAR_AGGREGATE_GET(name_) \
    MPIR_T_PVAR_AGGREGATE_GET_impl(name_)
#define MPIR_T_PVAR_AGGREGATE_INC(MODULE, name_, inc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_AGGREGATE_INC_impl(name_, inc_))

#define MPIR_T_PVAR_AGGREGATE_REGISTER_STATIC(MODULE, dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_AGGREGATE_REGISTER_STATIC_impl(dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_AGGREGATE_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_AGGREGATE_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_))

/* TIMER */

/* A timer actually has a twin, i.e., a counter, which counts how many times
 the timer is started/stopped so that we could know the average time
 for events measured. In our impl, the twins are exposed to MPI_T through the
 same name, but in two MPI_T_PVAR classes (timer and counter) and two data types
 (double and unsigned long long) respectively.
*/
#define MPIR_T_PVAR_DOUBLE_TIMER_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_TIMER_DECL_impl(name_))

#define MPIR_T_PVAR_DOUBLE_TIMER_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_TIMER_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_TIMER_INIT_VAR(MODULE, ptr_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_INIT_VAR_impl(ptr_))
#define MPIR_T_PVAR_TIMER_GET_VAR(MODULE, ptr_, buf) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_GET_VAR_impl(ptr_, buf))
#define MPIR_T_PVAR_TIMER_START_VAR(MODULE, ptr_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_START_VAR_impl(ptr_))
#define MPIR_T_PVAR_TIMER_END_VAR(MODULE, ptr_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_END_VAR_impl(ptr_))

#define MPIR_T_PVAR_TIMER_INIT(MODULE, name_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_INIT_impl(name_))
#define MPIR_T_PVAR_TIMER_GET(MODULE, name_, buf_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_GET_impl(name_, buf_))
#define MPIR_T_PVAR_TIMER_START(MODULE, name_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_START_impl(name_))
#define MPIR_T_PVAR_TIMER_END(MODULE, name_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_END_impl(name_))
#define MPIR_T_PVAR_TIMER_ADDR(name_) \
    MPIR_T_PVAR_TIMER_ADDR_impl(name_)

/* This macro actually register twins of a timer and a counter to MPIR_T */
#define MPIR_T_PVAR_TIMER_REGISTER_STATIC(MODULE, dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_TIMER_REGISTER_STATIC_impl(dtype_, name_, \
            verb_, bind_, flags_, cat_, desc_))

/* HIGHWATERMARK */
#define MPIR_T_PVAR_UINT_HIGHWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_HIGHWATERMARK_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG_HIGHWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_HIGHWATERMARK_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL_impl(name_))
#define MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_DECL_impl(name_))

#define MPIR_T_PVAR_UINT_HIGHWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_HIGHWATERMARK_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG_HIGHWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_HIGHWATERMARK_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_UINT_HIGHWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_HIGHWATERMARK_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG_HIGHWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_HIGHWATERMARK_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_DECL_EXTERN_impl(name_))

/* Embed type names in watermark operations because these operations need to
   know types of watermark to work, however we can't use typeof(ptr_) in C.
 */
#define MPIR_T_PVAR_UINT_HIGHWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_HIGHWATERMARK_INIT_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG_HIGHWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_HIGHWATERMARK_INIT_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG2_HIGHWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_HIGHWATERMARK_INIT_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_INIT_VAR_impl(ptr_, val_))

#define MPIR_T_PVAR_UINT_HIGHWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_HIGHWATERMARK_UPDATE_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG_HIGHWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_HIGHWATERMARK_UPDATE_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG2_HIGHWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_HIGHWATERMARK_UPDATE_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_UPDATE_VAR_impl(ptr_, val_))

#define MPIR_T_PVAR_UINT_HIGHWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_HIGHWATERMARK_INIT_impl(name_, val_))
#define MPIR_T_PVAR_ULONG_HIGHWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_HIGHWATERMARK_INIT_impl(name_, val_))
#define MPIR_T_PVAR_ULONG2_HIGHWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_HIGHWATERMARK_INIT_impl(name_, val_))
#define MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_INIT_impl(name_, val_))

#define MPIR_T_PVAR_UINT_HIGHWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_HIGHWATERMARK_UPDATE_impl(name_, val_))
#define MPIR_T_PVAR_ULONG_HIGHWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_HIGHWATERMARK_UPDATE_impl(name_, val_))
#define MPIR_T_PVAR_ULONG2_HIGHWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_HIGHWATERMARK_UPDATE_impl(name_, val_))
#define MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_HIGHWATERMARK_UPDATE_impl(name_, val_))

#define MPIR_T_PVAR_HIGHWATERMARK_REGISTER_STATIC(MODULE, dtype_, \
            name_, initval_, verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_HIGHWATERMARK_REGISTER_STATIC_impl(dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_HIGHWATERMARK_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_HIGHWATERMARK_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_))

/* LOWWATERMARK */
#define MPIR_T_PVAR_UINT_LOWWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_LOWWATERMARK_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG_LOWWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_LOWWATERMARK_DECL_impl(name_))
#define MPIR_T_PVAR_ULONG2_LOWWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_LOWWATERMARK_DECL_impl(name_))
#define MPIR_T_PVAR_DOUBLE_LOWWATERMARK_DECL(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_LOWWATERMARK_DECL_impl(name_))

#define MPIR_T_PVAR_UINT_LOWWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_LOWWATERMARK_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG_LOWWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_LOWWATERMARK_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_ULONG2_LOWWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_LOWWATERMARK_DECL_STATIC_impl(name_))
#define MPIR_T_PVAR_DOUBLE_LOWWATERMARK_DECL_STATIC(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_LOWWATERMARK_DECL_STATIC_impl(name_))

#define MPIR_T_PVAR_UINT_LOWWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_UINT_LOWWATERMARK_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG_LOWWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG_LOWWATERMARK_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_ULONG2_LOWWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_ULONG2_LOWWATERMARK_DECL_EXTERN_impl(name_))
#define MPIR_T_PVAR_DOUBLE_LOWWATERMARK_DECL_EXTERN(MODULE, name_) \
    PVAR_GATED_DECL(MODULE, MPIR_T_PVAR_DOUBLE_LOWWATERMARK_DECL_EXTERN_impl(name_))

#define MPIR_T_PVAR_UINT_LOWWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_LOWWATERMARK_INIT_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG_LOWWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_LOWWATERMARK_INIT_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG2_LOWWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_LOWWATERMARK_INIT_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_DOUBLE_LOWWATERMARK_INIT_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_LOWWATERMARK_INIT_VAR_impl(ptr_, val_))

#define MPIR_T_PVAR_UINT_LOWWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_LOWWATERMARK_UPDATE_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG_LOWWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_LOWWATERMARK_UPDATE_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_ULONG2_LOWWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_LOWWATERMARK_UPDATE_VAR_impl(ptr_, val_))
#define MPIR_T_PVAR_DOUBLE_LOWWATERMARK_UPDATE_VAR(MODULE, ptr_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_LOWWATERMARK_UPDATE_VAR_impl(ptr_, val_))

#define MPIR_T_PVAR_UINT_LOWWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_LOWWATERMARK_INIT_impl(name_, val_))
#define MPIR_T_PVAR_ULONG_LOWWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_LOWWATERMARK_INIT_impl(name_, val_))
#define MPIR_T_PVAR_ULONG2_LOWWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_LOWWATERMARK_INIT_impl(name_, val_))
#define MPIR_T_PVAR_DOUBLE_LOWWATERMARK_INIT(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_LOWWATERMARK_INIT_impl(name_, val_))

#define MPIR_T_PVAR_UINT_LOWWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_UINT_LOWWATERMARK_UPDATE_impl(name_, val_))
#define MPIR_T_PVAR_ULONG_LOWWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG_LOWWATERMARK_UPDATE_impl(name_, val_))
#define MPIR_T_PVAR_ULONG2_LOWWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_ULONG2_LOWWATERMARK_UPDATE_impl(name_, val_))
#define MPIR_T_PVAR_DOUBLE_LOWWATERMARK_UPDATE(MODULE, name_, val_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_DOUBLE_LOWWATERMARK_UPDATE_impl(name_, val_))

#define MPIR_T_PVAR_LOWWATERMARK_REGISTER_STATIC(MODULE, dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LOWWATERMARK_REGISTER_STATIC_impl(dtype_, name_, \
            initval_, verb_, bind_, flags_, cat_, desc_))

#define MPIR_T_PVAR_LOWWATERMARK_REGISTER_DYNAMIC(MODULE, dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_) \
    PVAR_GATED_ACTION(MODULE, MPIR_T_PVAR_LOWWATERMARK_REGISTER_DYNAMIC_impl(dtype_, name_, \
            addr_, count_, verb_, bind_, flags_, get_value_, get_count_, cat_, desc_))
#endif
