# MPICH Namespacing

Namespacing in MPICH uses a convention as shown in the following graph:

```
   MPI_(consts, types, etc.) MPL_                       OPA_
    ^                         ^                          ^
    |                         |                          |
     ________________________|_________________________/
      \                     MPIU_                      /
       \                      ^                       /
        _____________________|______________________/
         \                  MPID_                   /
          \                   ^                    /
           __________________|___________________/
                            MPIR_
```

The upper most layer is completely independent. The lower layers use the
functionality provided by the upper ones and thus depend on them.
