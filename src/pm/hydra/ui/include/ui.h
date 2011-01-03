/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

struct HYD_ui_info {
    char *prepend_regex;
    char *outfile_regex;
    char *errfile_regex;
};

extern struct HYD_ui_info HYD_ui_info;

#endif /* UI_H_INCLUDED */
