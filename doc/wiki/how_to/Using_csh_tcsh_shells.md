# Using csh and tcsh Shells with MPICH

This document provides shell-specific examples for csh and tcsh users. The main README.md focuses on bash/sh examples for simplicity.

## Configuration

### Configure MPICH

For csh and tcsh:
```csh
./configure --prefix=/home/<USERNAME>/mpich-install |& tee c.txt
```

## Building

### Build MPICH

For csh and tcsh:
```csh
make |& tee m.txt
```

If there were problems, do a `make clean` and then run make again with V=1:

```csh
make V=1 |& tee m.txt
```

## Installation

### Install the MPICH commands

For csh and tcsh:
```csh
make install |& tee mi.txt
```

## Environment Setup

### Add the bin subdirectory to your path

For csh and tcsh:
```csh
setenv PATH /home/<USERNAME>/mpich-install/bin:$PATH
```

Add this line to your startup script (.cshrc for csh, etc.).

## Notes

- Csh-like shells (csh and tcsh) accept `|&` for redirecting both stdout and stderr
- Bourne-like shells (sh and bash) accept `2>&1 |` for the same purpose
- The `setenv` command is used in csh/tcsh instead of `export` used in bash/sh