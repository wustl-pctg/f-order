THIS_FILE := $(lastword $(MAKEFILE_LIST))
include ../test.mk

SRC := $(wildcard test[0-9].cpp) $(wildcard test[0-9][0-9].cpp)
TARGETS := $(SRC:.cpp=)

all: $(TARGETS)

.SECONDEXPANSION:
$(TARGETS): $$@.o
	$(CXX) $< $(LDFLAGS) -o $@

.PHONY: all clean check $(TARGETS)

ccyellow=\033[0;33m
ccred=\033[31;01m
ccend=\033[0m

check: $(TARGETS)
	@printf "$(ccyellow) Begin basic tests ... $(ccend)\n"
	@for test in $(TARGETS); do \
		printf "$$test ..."; \
		./$$test ; \
		printf " passed!\n"; \
	done
	@printf "$(ccyellow) ... all passed! $(ccend)\n"

clean:
	rm -rf $(TARGETS) *.o core.*
