# This MUST be before any includes
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

include ../../../common.mk

COMMON_CXXFLAGS = -fcilktool -DTIMES_TO_RUN=1 -DTIMING_COUNT=1 -O3 -g -fcilkplus -fno-omit-frame-pointer -fcilk-no-inline -I../../src -I../../../llvm-cilk/include -std=c++11 -Wall
COMMON_LDFLAGS  = -ltcmalloc -lrt -ldl -lpthread -lnuma -lpthread -lm -lrt

FUTURE_CXXFLAGS = $(COMMON_CXXFLAGS) -I../../../cilkrtssuspend/runtime/config/x86 -I../../../cilkrtssuspend/runtime/sslib -I../../../rts/include
FUTURE_LDFLAGS 	= ../../../cilkrtssuspend/.libs/libcilkrts.a $(COMMON_LDFLAGS) 

VANILLA_CXXFLAGS = $(FUTURE_CXXFLAGS)
VANILLA_LDFLAGS = $(FUTURE_LDFLAGS)

.PHONY: default all clean
default: all

getoptions.o: ../util/getoptions.cpp
	$(CXX) $(FUTURE_CXXFLAGS) -c ../util/getoptions.cpp -o ../util/getoptions.o

ktiming.o: ../util/ktiming.cpp
	$(CXX) -O3 -c ../util/ktiming.cpp -o ../util/ktiming.o

%.o: %.cpp
	$(CXX) $(COMMON_CXXFLAGS) -I$(RTS_INCLUDE) -I$(RD_INCLUDE) -c $< -o $@

#TARGETS = lcs-se
#APPS = lcs-se

lcs-se: lcs-se.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(COMMON_CXXFLAGS) -c lcs-se.cpp -o lcs-se.o
	$(CXX) lcs-se.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o lcs-se $(COMMON_LDFLAGS)

TARGETS += lcs-fj
APPS += lcs-fj

lcs-fj: lcs.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(VANILLA_CXXFLAGS) -DNO_FUTURES -c lcs.cpp -o lcs-fj.o
	$(CXX) lcs-fj.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o lcs-fj $(VANILLA_LDFLAGS)

TARGETS += lcs-sf
APPS += lcs-sf

lcs-sf: lcs.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(FUTURE_CXXFLAGS) -DSTRUCTURED_FUTURES -c lcs.cpp -o lcs-sf.o
	$(CXX)  lcs-sf.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o lcs-sf $(FUTURE_LDFLAGS)

TARGETS += lcs-gf
APPS += lcs-gf

lcs-gf: lcs.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(FUTURE_CXXFLAGS) -DNONBLOCKING_FUTURES -c lcs.cpp -o lcs-gf.o
	$(CXX) lcs-gf.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o lcs-gf $(FUTURE_LDFLAGS)

#TARGETS += sw-se
#APPS += sw-se

sw-se: sw-se.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(COMMON_CXXFLAGS) -c sw-se.cpp -o sw-se.o
	$(CXX) sw-se.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o sw-se $(COMMON_LDFLAGS)

TARGETS += sw-fj
APPS += sw-fj

sw-fj: sw.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(VANILLA_CXXFLAGS) -DNO_FUTURES -c sw.cpp -o sw-fj.o
	$(CXX) sw-fj.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o sw-fj $(VANILLA_LDFLAGS)

TARGETS += sw-sf
APPS += sw-sf

sw-sf: sw.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(FUTURE_CXXFLAGS) -DSTRUCTURED_FUTURES -c sw.cpp -o sw-sf.o
	$(CXX)  sw-sf.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o sw-sf $(FUTURE_LDFLAGS)

TARGETS += sw-gf
APPS += sw-gf

sw-gf: sw.cpp ktiming.o getoptions.o rd_future.o
	$(CXX) $(FUTURE_CXXFLAGS) -DNONBLOCKING_FUTURES -c sw.cpp -o sw-gf.o
	$(CXX) sw-gf.o ../util/ktiming.o ../util/getoptions.o rd_future.o -o sw-gf $(FUTURE_LDFLAGS)

all: $(TARGETS)

clean:
	rm -f *.o $(APPS)
