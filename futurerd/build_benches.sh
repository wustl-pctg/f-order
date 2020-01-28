#!/bin/sh 

cd bench/hw-sf/cilk-futures
make mode=release rdalg=structured ftype=structured

cd -
cd bench/hw-gf/cilk-futures
make mode=release rdalg=nonblock ftype=nonblock

cd -
cd bench/sw-sf
make mode=release rdalg=structured ftype=structured

cd -
cd bench/sw-gf
make mode=release rdalg=nonblock ftype=nonblock

cd -
cd bench/sort
make mode=release rdalg=structured ftype=structured

cd -
cd bench/mm
make mode=release rdalg=structured ftype=structured

cd -

