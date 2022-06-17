## Coding Style Checker

(This page is mostly a stub)

The coding style checker used by the MPICH2 project is a simple yet
general purpose Perl program that applies a set of rules to all source
files in a directory tree.

The style checker is run every night by the script
`/home/MPI/nightly/updatetodo`. The command that is used is

```
  /home/gropp/projects/software/buildsys/src/codingcheck -conf=mpich2 \
         -skipfiles=src/mpid/globus,src/mpe2,src/pm/smpd,src/pm/ompd,src/pm/forker,src/mpid/mm,src/mpid/rdma,src/pm/mpd/examples,src/mpid/ch3/channels/rdma,src/mpid/dcmfd \
        -checktest src examples test
```

The options have the following meanings:

  - \-conf=mpich2: Apply the rules in `mpich2.pl` configuration file.
    This is located in the the `share/lib/coding` directory of the
    buildsys project (currently in
    `/home/gropp/projects/software/buildsys`). The contents of this are
    described below.
    \-skipfiles=list: Skip the files in the list. If the "file" is a
    directory, skip the directory and all of its members. This is used
    to skip directories from other projects that have their own (we
    hope) style guidelines and directories that are deprecated but still
    present in the source tree.
    \-checktest: Apply the style checks to files in directories with a
    parent directory of test. This applies the style checks to the
    MPICH2 test programs.
    src examples test:The remaining arguments are the files or
    directories to check.j

### The mpich2.pl configuration file

The `mpich2.pl` configuration file defines the tests to be applied to
the files in the MPICH2 project. In addition to some common tests, this
defines MPICH2-specific tests. These include:

  -
    CheckForPMPI: Check for the use of PMPI routines instead of NMPI.
    Allow the use of PMPI in certain files (by testing on the filename).

The default tests include

  - cppdefines: Check for proper use of the C preprocessor, including
    standard directives and name format
    comments: Check for valid comments (e.g., avoid // in C programs)
    and process special comments (see below)
    avoidfuncs: Check for functions whose use should normally be avoided
    (e.g., strcpy)
    funcnests: Check for possible problem is using the MPICH2 Nest_incr
    and decr
    notabs: Check that Fortran source files do not contain tabs.

While processing comments, the style checker looks for these special
forms:

  - copyright: Checks for the copyright
    preamble: Checks for the standard preamble (setting the Emacs
    language and indent amount)
    style: Checks for overrides for style checks, such as allowing the
    use of a function in the avoid-function list