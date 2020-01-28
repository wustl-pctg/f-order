PROJECT_HOME = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR = $(PROJECT_HOME)/build
COMPILER_HOME = $(PROJECT_HOME)/llvm-cilk
RUNTIME_HOME = $(PROJECT_HOME)/runtime
RUNTIME_LIB = $(PROJECT_HOME)/build/lib/libcilkrts.a

TOOL_DEBUG = 0
LTO ?= 1
STATS ?= 0

# Default future type to use
ftype ?= structured
# Default race detection algorithm
rdalg ?= structured
