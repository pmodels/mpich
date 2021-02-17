/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#ifndef MPIR_IMPL_H_INCLUDED
#define MPIR_IMPL_H_INCLUDED

int MPIR_Attr_delete_impl(MPIR_Comm *comm_ptr, int keyval);
int MPIR_Attr_get_impl(MPIR_Comm *comm_ptr, int keyval, void *attribute_val, int *flag);
int MPIR_Attr_put_impl(MPIR_Comm *comm_ptr, int keyval, void *attribute_val);
int MPIR_Comm_create_keyval_impl(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state);
int MPIR_Comm_delete_attr_impl(MPIR_Comm *comm_ptr, MPII_Keyval *comm_keyval_ptr);
int MPIR_Comm_get_attr_impl(MPIR_Comm *comm_ptr, int comm_keyval, void *attribute_val, int *flag, MPIR_Attr_type attr_type);
int MPIR_Comm_get_attr_impl(MPIR_Comm *comm_ptr, int comm_keyval, void *attribute_val, int *flag, MPIR_Attr_type attr_type);
int MPIR_Comm_set_attr_impl(MPIR_Comm *comm_ptr, MPII_Keyval *comm_keyval_ptr, void *attribute_val, MPIR_Attr_type attr_type);
int MPIR_Comm_set_attr_impl(MPIR_Comm *comm_ptr, MPII_Keyval *comm_keyval_ptr, void *attribute_val, MPIR_Attr_type attr_type);
int MPIR_Keyval_create_impl(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state);
int MPIR_Keyval_free_impl(int *keyval);
int MPIR_Type_create_keyval_impl(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state);
int MPIR_Type_delete_attr_impl(MPIR_Datatype *datatype_ptr, MPII_Keyval *type_keyval_ptr);
int MPIR_Type_get_attr_impl(MPIR_Datatype *datatype_ptr, int type_keyval, void *attribute_val, int *flag, MPIR_Attr_type attr_type);
int MPIR_Type_get_attr_impl(MPIR_Datatype *datatype_ptr, int type_keyval, void *attribute_val, int *flag, MPIR_Attr_type attr_type);
int MPIR_Type_set_attr_impl(MPIR_Datatype *datatype_ptr, MPII_Keyval *type_keyval_ptr, void *attribute_val, MPIR_Attr_type attr_type);
int MPIR_Type_set_attr_impl(MPIR_Datatype *datatype_ptr, MPII_Keyval *type_keyval_ptr, void *attribute_val, MPIR_Attr_type attr_type);
int MPIR_Win_create_keyval_impl(MPI_Win_copy_attr_function *win_copy_attr_fn, MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state);
int MPIR_Win_delete_attr_impl(MPIR_Win *win_ptr, MPII_Keyval *win_keyval_ptr);
int MPIR_Win_get_attr_impl(MPIR_Win *win_ptr, int win_keyval, void *attribute_val, int *flag, MPIR_Attr_type attr_type);
int MPIR_Win_get_attr_impl(MPIR_Win *win_ptr, int win_keyval, void *attribute_val, int *flag, MPIR_Attr_type attr_type);
int MPIR_Win_set_attr_impl(MPIR_Win *win_ptr, MPII_Keyval *win_keyval_ptr, void *attribute_val, MPIR_Attr_type attr_type);
int MPIR_Win_set_attr_impl(MPIR_Win *win_ptr, MPII_Keyval *win_keyval_ptr, void *attribute_val, MPIR_Attr_type attr_type);
int MPIR_Comm_compare_impl(MPIR_Comm *comm1_ptr, MPIR_Comm *comm2_ptr, int *result);
int MPIR_Comm_create_impl(MPIR_Comm *comm_ptr, MPIR_Group *group_ptr, MPIR_Comm **newcomm_ptr);
int MPIR_Comm_create_group_impl(MPIR_Comm *comm_ptr, MPIR_Group *group_ptr, int tag, MPIR_Comm **newcomm_ptr);
int MPIR_Comm_dup_impl(MPIR_Comm *comm_ptr, MPIR_Comm **newcomm_ptr);
int MPIR_Comm_dup_with_info_impl(MPIR_Comm *comm_ptr, MPIR_Info *info_ptr, MPIR_Comm **newcomm_ptr);
int MPIR_Comm_free_impl(MPIR_Comm *comm_ptr);
int MPIR_Comm_get_info_impl(MPIR_Comm *comm_ptr, MPIR_Info **info_used_ptr);
int MPIR_Comm_get_name_impl(MPIR_Comm *comm_ptr, char *comm_name, int *resultlen);
int MPIR_Comm_group_impl(MPIR_Comm *comm_ptr, MPIR_Group **group_ptr);
int MPIR_Comm_idup_impl(MPIR_Comm *comm_ptr, MPIR_Comm **newcomm_ptr, MPIR_Request **request_ptr);
int MPIR_Comm_idup_with_info_impl(MPIR_Comm *comm_ptr, MPIR_Info *info_ptr, MPIR_Comm **newcomm_ptr, MPIR_Request **request_ptr);
int MPIR_Comm_remote_group_impl(MPIR_Comm *comm_ptr, MPIR_Group **group_ptr);
int MPIR_Comm_set_info_impl(MPIR_Comm *comm_ptr, MPIR_Info *info_ptr);
int MPIR_Comm_split_impl(MPIR_Comm *comm_ptr, int color, int key, MPIR_Comm **newcomm_ptr);
int MPIR_Comm_split_type_impl(MPIR_Comm *comm_ptr, int split_type, int key, MPIR_Info *info_ptr, MPIR_Comm **newcomm_ptr);
int MPIR_Intercomm_create_impl(MPIR_Comm *local_comm_ptr, int local_leader, MPIR_Comm *peer_comm_ptr, int remote_leader, int tag, MPIR_Comm **newintercomm_ptr);
int MPIR_Intercomm_merge_impl(MPIR_Comm *intercomm_ptr, int high, MPIR_Comm **newintracomm_ptr);
int MPIR_Comm_shrink_impl(MPIR_Comm *comm_ptr, MPIR_Comm **newcomm_ptr);
int MPIR_Comm_agree_impl(MPIR_Comm *comm_ptr, int *flag);
int MPIR_Add_error_class_impl(int *errorclass);
int MPIR_Add_error_code_impl(int errorclass, int *errorcode);
int MPIR_Add_error_string_impl(int errorcode, const char *string);
int MPIR_Comm_call_errhandler_impl(MPIR_Comm *comm_ptr, int errorcode);
int MPIR_Comm_create_errhandler_impl(MPI_Comm_errhandler_function *comm_errhandler_fn, MPIR_Errhandler **errhandler_ptr);
int MPIR_Comm_get_errhandler_impl(MPIR_Comm *comm_ptr, MPI_Errhandler *errhandler);
int MPIR_Comm_set_errhandler_impl(MPIR_Comm *comm_ptr, MPIR_Errhandler *errhandler_ptr);
int MPIR_Errhandler_free_impl(MPIR_Errhandler *errhandler_ptr);
int MPIR_File_call_errhandler_impl(MPI_File fh, int errorcode);
int MPIR_File_create_errhandler_impl(MPI_File_errhandler_function *file_errhandler_fn, MPIR_Errhandler **errhandler_ptr);
int MPIR_File_get_errhandler_impl(MPI_File file, MPI_Errhandler *errhandler);
int MPIR_File_set_errhandler_impl(MPI_File file, MPIR_Errhandler *errhandler_ptr);
int MPIR_Win_call_errhandler_impl(MPIR_Win *win_ptr, int errorcode);
int MPIR_Win_create_errhandler_impl(MPI_Win_errhandler_function *win_errhandler_fn, MPIR_Errhandler **errhandler_ptr);
int MPIR_Win_get_errhandler_impl(MPIR_Win *win_ptr, MPI_Errhandler *errhandler);
int MPIR_Win_set_errhandler_impl(MPIR_Win *win_ptr, MPIR_Errhandler *errhandler_ptr);
int MPIR_Delete_error_class_impl(int errorclass);
int MPIR_Delete_error_code_impl(int errorcode);
int MPIR_Delete_error_string_impl(int errorcode);
int MPIR_Errhandler_create_impl(MPI_Comm_errhandler_function *comm_errhandler_fn, MPIR_Errhandler **errhandler_ptr);
int MPIR_Errhandler_get_impl(MPIR_Comm *comm_ptr, MPIR_Errhandler **errhandler_ptr);
int MPIR_Errhandler_set_impl(MPIR_Comm *comm_ptr, MPIR_Errhandler *errhandler_ptr);
int MPIR_Group_compare_impl(MPIR_Group *group1_ptr, MPIR_Group *group2_ptr, int *result);
int MPIR_Group_difference_impl(MPIR_Group *group1_ptr, MPIR_Group *group2_ptr, MPIR_Group **newgroup_ptr);
int MPIR_Group_excl_impl(MPIR_Group *group_ptr, int n, const int ranks[], MPIR_Group **newgroup_ptr);
int MPIR_Group_free_impl(MPIR_Group *group_ptr);
int MPIR_Group_incl_impl(MPIR_Group *group_ptr, int n, const int ranks[], MPIR_Group **newgroup_ptr);
int MPIR_Group_intersection_impl(MPIR_Group *group1_ptr, MPIR_Group *group2_ptr, MPIR_Group **newgroup_ptr);
int MPIR_Group_range_excl_impl(MPIR_Group *group_ptr, int n, int ranges[][3], MPIR_Group **newgroup_ptr);
int MPIR_Group_range_incl_impl(MPIR_Group *group_ptr, int n, int ranges[][3], MPIR_Group **newgroup_ptr);
int MPIR_Group_translate_ranks_impl(MPIR_Group *group1_ptr, int n, const int ranks1[], MPIR_Group *group2_ptr, int ranks2[]);
int MPIR_Group_union_impl(MPIR_Group *group1_ptr, MPIR_Group *group2_ptr, MPIR_Group **newgroup_ptr);
int MPIR_Info_delete_impl(MPIR_Info *info_ptr, const char *key);
int MPIR_Info_dup_impl(MPIR_Info *info_ptr, MPIR_Info **newinfo_ptr);
int MPIR_Info_free_impl(MPIR_Info *info_ptr);
int MPIR_Info_get_impl(MPIR_Info *info_ptr, const char *key, int valuelen, char *value, int *flag);
int MPIR_Info_get_nkeys_impl(MPIR_Info *info_ptr, int *nkeys);
int MPIR_Info_get_nthkey_impl(MPIR_Info *info_ptr, int n, char *key);
int MPIR_Info_get_valuelen_impl(MPIR_Info *info_ptr, const char *key, int *valuelen, int *flag);
int MPIR_Info_set_impl(MPIR_Info *info_ptr, const char *key, const char *value);
int MPIR_Abort_impl(MPIR_Comm *comm_ptr, int errorcode);
int MPIR_Finalize_impl(void);
int MPIR_Init_impl(int *argc, char ***argv);
int MPIR_Init_thread_impl(int *argc, char ***argv, int required, int *provided);
int MPIR_T_category_get_categories_impl(int cat_index, int len, int indices[]);
int MPIR_T_category_get_cvars_impl(int cat_index, int len, int indices[]);
int MPIR_T_category_get_pvars_impl(int cat_index, int len, int indices[]);
int MPIR_T_cvar_handle_alloc_impl(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle, int *count);
int MPIR_T_cvar_read_impl(MPI_T_cvar_handle handle, void *buf);
int MPIR_T_cvar_write_impl(MPI_T_cvar_handle handle, const void *buf);
int MPIR_T_pvar_handle_alloc_impl(MPI_T_pvar_session session, int pvar_index, void *obj_handle, MPI_T_pvar_handle *handle, int *count);
int MPIR_T_pvar_handle_free_impl(MPI_T_pvar_session session, MPI_T_pvar_handle *handle);
int MPIR_T_pvar_read_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int MPIR_T_pvar_readreset_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int MPIR_T_pvar_session_create_impl(MPI_T_pvar_session *session);
int MPIR_T_pvar_session_free_impl(MPI_T_pvar_session *session);
int MPIR_T_pvar_write_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf);
int MPIR_Sendrecv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPIR_Comm *comm_ptr, MPI_Status *status);
int MPIR_Sendrecv_replace_impl(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPIR_Comm *comm_ptr, MPI_Status *status);

#endif /* MPIR_IMPL_H_INCLUDED */
