# Collective Selection Framework

The collective selection framework (Csel) is intended to allow users or
system administrators to provide a text-format selection logic that is
translated to C-based selection at runtime. We use json-formatted input
files for user-input.

## Collective Selection Framework Components

There are two components of the collective selection framework:
operators and containers. Operators are branching code needed for the
selection logic: they just give a true/false answer (e.g., "is my
communicator size lesser than 8"). Containers are the final algorithms
or compositions that need to be called for a given collective in a
particular case.

## Csel class

The csel class provides the logic for reading the json file and
converting it to a tree of operators and containers. The leaf nodes in a
tree are always containers. Containers can only be leaf nodes in the
tree.

The csel class understands operators, but not the containers, i.e., it
treats containers as simple "void \*" opaque buffers. Containers are
specific to the caller of the csel class, and the caller would need to
provide the logic to parse the json object and create the container.

## Pruning the selection tree

The selection tree can be pruned during communicator creation. Such
pruning would get rid of the branches that can be statically determined
at communicator creation time (e.g., branches based on the size of the
communicator). This would reduce the tree parsing overhead during the
actual collective call.

## Json file

The json file would contain the selection logic as a series of
operator/containers. The following operator types are supported:

- `coll_size <= [number]`
- `coll_size = pow2`
- `coll_size = any`
- `msg_size <= [number]`
- `msg_size = any`
- `comm_type = intra`
- `comm_type = inter`
- `collective = [bcast|barrier|...]`

A few restrictions apply:

- All siblings in any given subtree of the json tree have to be of the
  same type.
- It is preferable (though not required) to have communicator-creation
  time information (e.g., comm_size) be at the outer levels of the
  json file (upper levels of the tree), so the tree can be effectively
  pruned at communicator creation time.
- The "collective" operator is an exception and can reside wherever
  the user likes. Pruning is done both before and after the collective
  operator.
- All subtrees of the json file have to be complete. For example, ne
  cannot have a subtree that only deals with small message sizes, but
  not large message sizes.
