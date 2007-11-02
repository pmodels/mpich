/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#pragma once

class CExampleNode
{
public:
    CExampleNode(void);
    ~CExampleNode(void);

    double xmin, ymin, xmax, ymax;
    int max_iter;
    CExampleNode *next;
};
