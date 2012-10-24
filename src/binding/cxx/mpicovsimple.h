/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * This is a *very* simple tool for basic coverage analysis.  
 * This is intended as a stop-gap until gcov works with the C++ files
 * used in the MPICH binding of C++ (as of 2/23/2004, gcov aborts when
 * processing the coverage files produced by g++ for the MPICH C++
 * binding).
 */
/* style: c++ header */

#ifndef MPIX_SIMPLECOVERAGE_H
#define MPIX_SIMPLECOVERAGE_H

typedef struct _covinfo {
    char   *name;               // Routine name (or block)
    int    argcount;            // Number of arguments 
    int    count;               // Number of times called
    char   *sourceFile;         // Name of source file
    int    firstLine, lastLine; // source lines for block
    struct _covinfo *fLink, *bLink;
} covinfo;

class MPIX_Coverage {
private:
  covinfo *head;
  covinfo *findOrInsert( const char name[], int argcount );   // return an initialize record
 
public:
    // New and delete
    MPIX_Coverage(void) { head = 0; }
    //
    void Init( void );
    void Add( const char name[], int argcnt, const char file[], int line );
    void AddEnd( const char name[], int argcnt, const char file[], int line );
    int FileMerge( const char filename[] );
};

extern MPIX_Coverage MPIR_Cov;

#endif
