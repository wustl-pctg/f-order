THIS_FILE := $(lastword $(MAKEFILE_LIST))
include ../bench.mk

BENCH = cilksort-future
ALLTARGETS = $(BENCH:=-rd) $(BENCH:=-inst) $(BENCH:=-reach) $(BENCH:=-base)
TARGETS = $(BENCH:=-$(btype))

ifeq ($(btype),inst)
  INST = yes
endif
ifeq ($(btype),rd)
  INST = yes
endif
ifeq ($(INST),yes)
  GETOPT = ../util/getoptions-inst.o
else
  GETOPT = ../util/getoptions.o
endif

.PHONY: all clean reallyclean $(BENCH) $(BENCH:=-all)
all: $(TARGETS)

.SECONDEXPANSION:
$(BENCH:=-serial): $$@.o ../util/getoptions.o
	$(CXX) $^ $(LDFLAGS) -o $@

$(BENCH:=-all):
	$(MAKE) mode=$(mode) rdalg=$(rdalg) ftype=$(ftype) btype=rd $(@:-all=)
	$(MAKE) mode=$(mode) rdalg=$(rdalg) ftype=$(ftype) btype=inst $(@:-all=)
	$(MAKE) mode=$(mode) rdalg=$(rdalg) ftype=$(ftype) btype=reach $(@:-all=)
	$(MAKE) mode=$(mode) rdalg=$(rdalg) ftype=$(ftype) btype=base $(@:-all=)

INC += -I../util
CFLAGS += $(APPFLAGS)
CXXFLAGS += $(APPFLAGS)

.SECONDEXPANSION:
$(BENCH): $$@-$(btype)
$(ALLTARGETS): $$@.o $(GETOPT) | $(RDLIB)
	$(CXX) $^ $(LDFLAGS) -o $@

# There seems to be a very strange error when using both LTO and -O1
# optimizations. It causes a segfault during get_options, but I cannot
# find anything that is really a problem. Interesting, if you include
# getoptions.cpp instead of getoptions.hpp, the problem goes away, so
# the process of linking is somehow involved (bug in gold?). Anyway,
# the code is not on the critical path, so just don't use
# optimizations for this file.
../util/getoptions.o: ../util/getoptions.cpp
	$(CXX) -O0 $(CLAGS) -O0 -o $@ -c $<

ifeq ($(btype),all)
clean:
	rm -rf core.* ../util/*.o $(ALLTARGETS) $(ALLTARGETS:=.o)
else
clean:
	rm -f core.* ../util/*.o $(TARGETS) $(TARGETS:=.o)
endif

reallyclean:
	rm -f *.o ../util/*.o core.* $(ALLTARGETS)
