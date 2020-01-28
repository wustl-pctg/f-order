TEST_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
include $(TEST_DIR)/../common.mk

# APP_TOOL_ONLY_FLAGS = -fcilktool
# APP_RD_FLAGS = $(APP_TOOL_ONLY_FLAGS) -fsanitize=thread

INC += -I$(PROJECT_HOME)/src/ # for future.hpp
INC += -I$(BUILD_DIR)/include # for runtime include
INC += -I$(BENCH_DIR)         # for util stuff

RDLIB = $(LIB_DIR)/librd-$(ftype).a

# INC += -I$(PROJECT_HOME)/llvm-cilk/include # for runtime include
# INC += -I$(PROJECT_HOME)/cilkrts/include/

APPFLAGS = -fcilkplus -fcilk-no-inline
APPFLAGS += -fcilktool -DRACE_DETECT
APPFLAGS += -fsanitize=thread #-fsanitize-blacklist=../../blacklist.txt

# This really only matters for optimized builds, which won't use a
# frame pointer register. This makes __builtin_frame_address return 0,
# which we use in __tsan_func_exit to clear the shadow stack memory.
APPFLAGS += -fno-omit-frame-pointer

CFLAGS   += $(APPFLAGS)
CXXFLAGS += $(APPFLAGS)
LDFLAGS  += -ldl -lpthread $(RDLIB) $(RUNTIME_LIB:.a=-cilktool.a)
