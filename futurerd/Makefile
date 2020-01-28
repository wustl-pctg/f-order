include common.mk

TARGET = $(LIB_DIR)/libfuturerd.a

.DEFAULT_GOAL := lib

lib:
	$(MAKE) -C ./src

check: $(TARGET)
	$(MAKE) -C ./test/basic check

clean:
	$(MAKE) -C ./src clean
	$(MAKE) -C ./test/basic clean

# Future targets
# doc: Generate documentation
# perf: Run benchmarks and generate a report
# prof: Compile code with profiling enabled, run benchmarks and
#   generate profiling report

# Modes for each compilation target:
# release/perf: all optimizations enabled
# debug: no (of few) optimizations, lots of debugging features
# prof: optimizations enabled but profiling also on
