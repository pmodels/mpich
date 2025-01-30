/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_SEQ_POST_H_INCLUDED
#define YAKSURI_SEQ_POST_H_INCLUDED

int yaksuri_seq_init_hook(void);
int yaksuri_seq_finalize_hook(void);
int yaksuri_seq_type_create_hook(yaksi_type_s * type);
int yaksuri_seq_type_free_hook(yaksi_type_s * type);
int yaksuri_seq_info_create_hook(yaksi_info_s * info);
int yaksuri_seq_info_free_hook(yaksi_info_s * info);
int yaksuri_seq_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                                   unsigned int vallen);

int yaksuri_seq_pup_is_supported(yaksi_type_s * type, yaksa_op_t op, bool * is_supported);
int yaksuri_seq_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                      yaksi_info_s * info, yaksa_op_t op);
int yaksuri_seq_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                        yaksi_info_s * info, yaksa_op_t op);

#endif /* YAKSURI_SEQ_H_INCLUDED */
