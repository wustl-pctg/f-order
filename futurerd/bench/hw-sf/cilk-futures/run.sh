#!/bin/sh
# The first param is the number of frames to process, 
# second is num of omp threads
# original input: 20 4
# full input: 104 1

N=10
if [ "$1" != "" ]; then N=$1; fi
./hw-base ../data/test.avi $N 1
./hw-reach ../data/test.avi $N 1
./hw-inst ../data/test.avi $N 1
./hw-rd ../data/test.avi $N 1
