TOOL_HOME:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
include $(TOOL_HOME)/config.mk

TOOL_NAME = rd
LIB_DIR ?= $(TOOL_HOME)/lib
INC_DIR ?= $(TOOL_HOME)/include

CC = $(COMPILER_HOME)/bin/clang
CXX = $(COMPILER_HOME)/bin/clang++

OPT_FLAG ?= -O3
TOOL_DEBUG ?= 0
CILKFLAGS += -fcilkplus -fcilk-no-inline
INC = -I$(INC_DIR) -I$(RUNTIME_INTERNAL)/include
# -fno-omit-frame-pointer required to work properly
FLAGS += -Wall -fno-omit-frame-pointer $(CILKFLAGS) $(INC)
FLAGS += $(OPT_FLAG) -g -DTOOL_DEBUG=$(TOOL_DEBUG)
CFLAGS += $(FLAGS) -std=c99
CXXFLAGS += $(FLAGS) -std=c++11
LDFLAGS += 
ARFLAGS +=

## Each C source file will have a corresponding file of prerequisites.
## Include the prerequisites for each of our C source files.

# This rule generates a file of prerequisites (i.e., a makefile)
# called name.d from a C source file name.c.
%.d: CFLAGS += -MM -MP
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) $(CFLAGS) -MF $@.$$$$ $<; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# This rule generates a file of prerequisites (i.e., a makefile)
# called name.d from a CPP source file name.cpp.
%.d: CXXFLAGS += -MM -MP
%.d: %.cpp
	@set -e; rm -f $@; \
	$(CXX) $(CXXFLAGS) -MF $@.$$$$ $<; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
 
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/$@ -c $<

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(LIB_DIR)/lib%.a: $(OBJ)
	ar rcs $(ARFLAGS) $@ $(OBJ)

$(LIB_DIR)/lib%.so: $(OBJ)
	$(CC) $(OBJ) -shared -o $@

# Don't delete any intermediate files
.SECONDARY:
