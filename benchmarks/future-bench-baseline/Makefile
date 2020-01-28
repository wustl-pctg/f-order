###########################################################################
# NOTE: Thou shalt add thine build targets to the TARGETS designator, and #
#       thou shalt add thine resulting executables to the APPS designator,#
#       lest thou desire to be charged with polluting the systems of      #
#       thine fellows.                                                    #
###########################################################################

# This MUST be before any includes
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

include ../../common.mk

COMMON_CXXFLAGS = -DTIMES_TO_RUN=1 -DTIMING_COUNT=1 -O3 -g -fcilkplus -fno-omit-frame-pointer -fcilk-no-inline -D__CILKRTS_ABI_VERSION=1 -std=c++11 -Wall
COMMON_LDFLAGS  = -ltcmalloc -lrt -ldl -lpthread -lnuma -lpthread -lm -lrt

#FUTURE_CXXFLAGS = $(COMMON_CXXFLAGS) -I../cilkrtssuspend/runtime/config/x86 -I../cilkrtssuspend/runtime/sslib -I../cilkrtssuspend/include
#FUTURE_LDFLAGS 	= ../cilkrtssuspend/.libs/libcilkrts.a $(COMMON_LDFLAGS) 
FUTURE_CXXFLAGS = $(COMMON_CXXFLAGS) -I$(RUNTIME_HOME)/include
FUTURE_LDFLAGS 	= $(RTS_LIB) $(COMMON_LDFLAGS) 

#VANILLA_CXXFLAGS = $(COMMON_CXXFLAGS) -I../cilkplus-rts/runtime/config/x86 -I../cilkplus-rts/runtime/sslib -I../cilkplus-rts/include
#VANILLA_LDFLAGS  = ../cilkplus-rts/.libs/libcilkrts.a $(COMMON_LDFLAGS) 
VANILLA_CXXFLAGS = $(FUTURE_CXXFLAGS)
VANILLA_LDFLAGS  = $(FUTURE_LDFLAGS)

.PHONY: default all clean
default: all

getoptions.o: getoptions.cpp
	$(CXX) $(FUTURE_CXXFLAGS) -c getoptions.cpp -o getoptions.o

ktiming.o: ktiming.c
	$(CXX) -O2 -c ktiming.c -o ktiming.o

%.o: %.cpp
	$(CXX) $(COMMON_CXXFLAGS) -I$(RTS_INCLUDE) -I$(RD_INCLUDE) -c $< -o $@

TARGETS = fib-se
APPS = fib-se

fib-se: ktiming.o rd_future.o
	$(CXX) $(FUTURE_CXXFLAGS) -c fib_cilkplus_se.cpp -o fib_se.o
	$(CXX) fib_se.o ktiming.o rd_future.o -o fib-se $(FUTURE_LDFLAGS)

TARGETS += fib-fj
APPS += fib-fj

fib-fj: ktiming.o rd_future.o
	$(CXX) $(VANILLA_CXXFLAGS) -c fib_cilkplus.cpp -o fib-fj.o
	$(CXX) fib-fj.o ktiming.o rd_future.o -o fib-fj $(VANILLA_LDFLAGS)

#TARGETS += fib-future-macro
#APPS += fib-future-macro

#fib-future-macro: ktiming.o tsan.o shadow_stack.o stat_util.o mem_access.o list.o
#	$(CXX) $(FUTURE_CXXFLAGS) -c fib_cilkfut.cpp -o fib-future-macro.o
#	$(CXX) fib-future-macro.o ktiming.o tsan.o shadow_stack.o stat_util.o mem_access.o list.o -o fib-future-macro $(FUTURE_LDFLAGS)

TARGETS += fib-sf
APPS += fib-sf

fib-sf: ktiming.o rd_future.o
	$(CXX) $(FUTURE_CXXFLAGS) -c handcomp_fib_cilkfut.cpp -o fib-sf.o
	$(CXX) fib-sf.o ktiming.o rd_future.o -o fib-sf $(FUTURE_LDFLAGS)

TARGETS += smm-se
APPS += smm-se

smm-se: ktiming.o rd_future.o getoptions.o strassen.cpp
	$(CXX) $(FUTURE_CXXFLAGS) -c strassen-se.cpp -o smm-se.o
	$(CXX) smm-se.o getoptions.o ktiming.o rd_future.o -o smm-se $(FUTURE_LDFLAGS)

TARGETS += smm-fj
APPS += smm-fj

smm-fj: ktiming.o rd_future.o getoptions.o strassen.cpp
	$(CXX) $(VANILLA_CXXFLAGS) -c strassen.cpp -o smm-fj.o
	$(CXX) smm-fj.o getoptions.o ktiming.o rd_future.o -o smm-fj $(FUTURE_LDFLAGS)
	#make run-strass

TARGETS += smm-sf
APPS += smm-sf
    
smm-sf: ktiming.o rd_future.o getoptions.o strassen-future.cpp
	$(CXX) $(FUTURE_CXXFLAGS) -c strassen-future.cpp -o smm-sf.o
	$(CXX) smm-sf.o getoptions.o ktiming.o rd_future.o -o smm-sf $(FUTURE_LDFLAGS)
	#make run-strass-fut

TARGETS += mm-se
APPS += mm-se

mm-se: matmul-se.cpp ktiming.o rd_future.o getoptions.o
	$(CXX) $(FUTURE_CXXFLAGS) -c matmul-se.cpp -o mm-se.o
	$(CXX) mm-se.o getoptions.o ktiming.o rd_future.o -o mm-se $(FUTURE_LDFLAGS)

TARGETS += mm-fj
APPS += mm-fj

mm-fj: matmul.cpp ktiming.o rd_future.o getoptions.o
	$(CXX) $(VANILLA_CXXFLAGS) -c matmul.cpp -o mm-fj.o
	$(CXX) mm-fj.o getoptions.o ktiming.o rd_future.o -o mm-fj $(VANILLA_LDFLAGS)
	##make run-matmul

TARGETS += mm-sf
APPS += mm-sf

mm-sf: matmul-future.cpp ktiming.o rd_future.o getoptions.o
	$(CXX) $(FUTURE_CXXFLAGS) -c matmul-future.cpp -o mm-sf.o
	$(CXX)  mm-sf.o getoptions.o ktiming.o rd_future.o -o mm-sf $(FUTURE_LDFLAGS)
	##make run-matmul-future

TARGETS += sort-se
APPS += sort-se

sort-se: cilksort-se.cpp ktiming.o rd_future.o getoptions.o
	$(CXX) $(FUTURE_CXXFLAGS) -c cilksort-se.cpp -o sort-se.o
	$(CXX) sort-se.o getoptions.o ktiming.o rd_future.o -o sort-se $(FUTURE_LDFLAGS)

TARGETS += sort-fj
APPS += sort-fj

sort-fj: cilksort.cpp ktiming.o rd_future.o getoptions.o
	$(CXX) $(VANILLA_CXXFLAGS) -c cilksort.cpp -o sort-fj.o
	$(CXX) sort-fj.o getoptions.o ktiming.o rd_future.o -o sort-fj $(VANILLA_LDFLAGS)

TARGETS += sort-sf
APPS += sort-sf

sort-sf: cilksort-future.cpp ktiming.o rd_future.o getoptions.o
	$(CXX) $(FUTURE_CXXFLAGS) -c cilksort-future.cpp -o sort-sf.o
	$(CXX) sort-sf.o getoptions.o ktiming.o rd_future.o -o sort-sf $(FUTURE_LDFLAGS)

###########################################################################
# Though shalt not cross this line lest thou knowest what thou art doing! #
###########################################################################

all: $(TARGETS)

clean:
	rm -f *.o $(APPS)
