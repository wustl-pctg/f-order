#!/bin/sh 

cp rts/include/internal/future_base_base.h rts/include/internal/future_base.h

cd benchmarks/future-bench-baseline

make sort-sf
make smm-sf
make mm-sf

cd -
cd benchmarks/futurerd-bench/basic-baseline

make sw-sf
make sw-gf

cd -
cd benchmarks/futurerd-bench/heartWallTracking-baseline/cilk-futures

make hw-sf
make hw-gf

cd -
cd benchmarks/ferret/src
cp Makefile_ori Makefile
make full_clean
make cilk-future-base

###############################################

cd -
cd benchmarks/future-bench

make sort-sf
make smm-sf
make mm-sf

cd -
cd benchmarks/ferret/src
cp Makefile_ori Makefile
make full_clean
make cilk-future

cd -
cd benchmarks/futurerd-bench/heartWallTracking/cilk-futures
make hw-sf
make hw-gf

cd -
cd benchmarks/futurerd-bench/basic
make sw-sf
make sw-gf

#############################################

cd -
cp rts/include/internal/future_base_full.h rts/include/internal/future_base.h

cd benchmarks/future-bench
make sort-sf-full
make smm-sf-full
make mm-sf-full


cd -
cd benchmarks/ferret/src
cp Makefile_full Makefile
make full_clean
make cilk-future

cd -
cd benchmarks/futurerd-bench/heartWallTracking/cilk-futures
make hw-sf-full
make hw-gf-full

cd -
cd benchmarks/futurerd-bench/basic
make sw-sf-full
make sw-gf-full


