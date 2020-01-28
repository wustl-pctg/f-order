ifeq ($(rdalg),)
  rdalg=both
endif
include ../common.mk

# Need this flag or the race detector won't do anything
TOOLFLAGS += -DRACE_DETECT -DSTATS=$(STATS)

# Basic tool functionality
SRC := driver.cpp rd.cpp

# Shadow memory
SRC += mem_access.cpp shadow_mem.cpp

# Reachability
SRC += reach_gen.cpp reach_nonblock.cpp # Don't need for structured
SRC += reach_structured.cpp # Even nonblocking needs this

# Other helpers
SRC += $(addprefix utils/,union_find.cpp bitvector.cpp bitsums.cpp)
SRC += debug.cpp spbag.cpp

OBJ := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))
LIB_DIR = $(BUILD_DIR)/lib
TARGET = $(LIB_DIR)/librd-$(rdalg).a

ifeq ($(rdalg),both)
	.DEFAULT_GOAL := both
else
	.DEFAULT_GOAL := $(TARGET)
  ifeq ($(rdalg),structured)
    TOOLFLAGS += -DSTRUCTURED_FUTURES
  else ifeq ($(rdalg),nonblock)
    TOOLFLAGS += -DNONBLOCKING_FUTURES
  else
    $(error "Invalid RD algorithm")
  endif
endif

CFLAGS += $(TOOLFLAGS)
CXXFLAGS += $(TOOLFLAGS) -fno-exceptions -fno-rtti

.PHONY: both

both:
	$(MAKE) clean
	$(MAKE) mode=$(mode) rdalg=structured
	$(MAKE) clean
	$(MAKE) mode=$(mode) rdalg=nonblock

# Mostly for cilk/internal includes
INC += -I$(PROJECT_HOME)/cilkrts/include

$(TARGET): $(OBJ) $(LIB_DIR) $(SRC)

$(LIB_DIR):
	if [ ! -e $(LIB_DIR) ]; then mkdir -p $(LIB_DIR); fi

clean:
	rm -rf $(OBJ)

reallyclean:
	rm -rf $(OBJ) $(LIB_DIR)/librd-*.a
