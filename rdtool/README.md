# CRACER

An asymptotically efficient race detector for Cilk Plus programs. It
requires a version of Cilk Plus that supports both -fno-inline-detach (or -fcilk-no-inline)
and the Batcher runtime system.

You should defined a top-level file `config.mk` which must include definitions for the compiler home, internal runtime headers, and runtime library location. For example:

	COMPILER_HOME = $(HOME)/llvm-cilk
	RUNTIME_INTERNAL = $(HOME)/devel/batcher/cilkplusrts
	RUNTIME_LIB = $(HOME)/llvm-cilk/lib/x86_64/libcilkrts.a
	OPT_FLAG = -O3 -DSTATS=0
	TOOL_DEBUG = 0
	LDFLAGS +=
	ARFLAGS =

If you'd also like to compare against cilksan or cilkscreen, or use a different malloc, use:

	CILKSAN_HOME := $(HOME)/devel/cilksan
	MALLOC=-ltcmalloc # Can also be empty or "-ltbbmalloc_proxy"
	ICC=$(HOME)/intel/bin/icc
	INTEL_LIB=$(HOME)/intel/lib/intel64

If you want link-time optimization, you'll need to add `-flto` to `OPT_FLAG` and `LDFLAGS`, plus set `ARFLAGS` to:

	ARFLAGS=--plugin $(COMPILER_HOME)/lib/LLVMgold.so

For this to work, the gold linker should be installed in the system path as `ld`, llvm/clang must have been compiled to use gold, and the Cilk Plus runtime must have been compiled with `-flto`.

In order for the tool to work correctly, apps need to be compiled with

	 -fno-omit-frame-pointer

The tool and runtime also need this, I believe.


## TODO 

- Split this off into a separate project repo -- it should not be part of Batcher.
- Debug performance issues with some benchmarks
- Cleanup the OM data structure. Possibly write in C++. It may even be
  worth it to split this into a separate project; I know of no other
  OM data structures implemented in C/C++, so others might find it
  useful.
