# Helpful Compiler Flags For Testing

A list of compiler warnings, facilities, and other features
that might help you track down that strange bug.

## Integer overflow

We have many spots where the product of one or more 64 bit values is
stored in a 32 bit value. What happens when the product is larger than
32 bits? usually the result is a very large negative number and a very
confused MPICH library

- `-Wshorten-64-to-32` - (Clang only) Generates a compile-time warning for 
  any case where, as the name implies, a 64 bit value is getting jammed into
  a 32 bit value. If you can prove this is fine, an explicit cast will silence
  this warning
- `-fsanitize=integer` - (Clang only) Will generate a run-time error whenever an integer is
  treated strangely. From the [manual](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html):
  "Checks for undefined or suspicious integer behavior (e.g.
  unsigned integer overflow). Enables signed-integer-overflow, unsigned-integer-overflow,
  shift, integer-divide-by-zero, and implicit-integer-truncation." May require you to add `-lubsan`
  to your LIBS variable when you configure mpich. Will complain about unsigned int overflowing, too. 
  Though it can be surprising, this is **defined** behavior, and the places in MPICH that overflow an
  unsigned int do so deliberately. `-fno-sanitize=unsigned-integer-overflow` will eliminate the
  warnings for the seven or so locations where this happens.
- `-fsanitize=signed-integer-overflow` - (GCC only) GCC's integer sanitizer does not have a general integer
  category like clang does, but you can get the case we most often care about with this flag.
- `-fsanitize=undefined` - (GCC and Clang) Very broad category of any undefined behavior. 
  Untested: if you try this, let me know how many sites are flagged.
