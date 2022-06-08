# Best Practices

This page describes the best practices for MPICH developers to make life
easier for developers integrating with our code base. This document is a
slightly modified version of the suggestions from Microsoft developers
who frequently integrate with the MPICH code base.

  - Avoid renaming and modifying files in a single check-in. Rename the
    files in one check-in and modify the files in subsequent check-ins.
  - Avoid moving and modifying functions between files in a single
    check-in. Move functions between files in one check-in and modify
    the functions in subsequent check-ins.
  - Avoid commenting out or `#ifdef` out a section of code and rewrite
    it as new code. Remove the obsolete code and add the new code in the
    same check-in.
  - Avoid mixing source code beautification, E.g. limiting source code
    to 80 columns, with actual code change.
  - Avoid using tabs, use spaces instead.
  - Avoid making multiple logical changes in a single check-in. Make
    separate check-ins for each logical change.
  - Avoid silently breaking a functionality and fixing it after many
    check-ins. Use a separate branch for the broken code and merge it to
    the trunk after fixing the functionality.
  - Avoid retaining obsolete fields in a structure, unused variables and
    unused static functions. Remove the obsolete code in the same
    check-in where you stop using them.
