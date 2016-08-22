/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4I_UTIL_H_INCLUDED
#define CH4I_UTIL_H_INCLUDED

void MPIDI_CH4I_map_create(void **_map);
void MPIDI_CH4I_map_destroy(void *_map);
void MPIDI_CH4I_map_set(void *_map, uint64_t id, void *val);
void MPIDI_CH4I_map_erase(void *_map, uint64_t id);
void *MPIDI_CH4I_map_lookup(void *_map, uint64_t id);

#endif /* CH4I_UTIL_H_INCLUDED */
