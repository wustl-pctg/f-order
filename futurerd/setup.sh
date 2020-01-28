#!/bin/sh
# Script to setup everything, for reproducible results.
set -e
BASE=$(pwd)
COMPDIR=$BASE/llvm-cilk/bin

mkdir -p build

# If you have g++-6+ installed, must use this:
#export CPLUS_INCLUDE_PATH=/usr/include:/usr/include/c++/5 

## Setup compiler (LLVM)
./build-llvm-linux.sh

# Setup runtime system
git clone -b futurerd https://gitlab.com/wustl-pctg/cilkplus-rts runtime
cd runtime
autoreconf -i
cd -

#cd bench/pipeline/dedup/inputs
#wget http://parsec.cs.princeton.edu/download/3.0/parsec-3.0-input-native.tar.gz
#wget http://parsec.cs.princeton.edu/download/3.0/parsec-3.0-input-sim.tar.gz
#wget http://parsec.cs.princeton.edu/download/3.0/parsec-3.0-core.tar.gz
#tar vxzf parsec-3.0-input-native.tar.gz
#tar vxzf parsec-3.0-input-sim.tar.gz
#tar vxzf parsec-3.0-core.tar.gz
#mv parsec-3.0/pkgs/kernels/dedup/inputs/*.tar .
#./unpack.sh
#cd -

cd bench
./run.sh
cd -
