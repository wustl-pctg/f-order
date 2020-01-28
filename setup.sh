#!/bin/sh

./build-llvm-linux.sh

RTS_PATH=`pwd`/rts
LLVM_HOME=`pwd`/llvm-cilk

cd cilkrtssuspend
libtoolize
aclocal
automake --add-missing
autoconf
./configure --prefix=$RTS_PATH CC=$LLVM_HOME/bin/clang CXX=$LLVM_HOME/bin/clang++
make -j
make install

cd -
cp rts/include/cilk/rdf.h rts/include/cilk/rd_future.h
