#!/bin/bash
set -e

# Only when g++-6+ is installed
#export CPLUS_INCLUDE_PATH=/usr/include:/usr/include/c++/5 

#PROGS=(lcs sw matmul_z hw dedup merge)
source remake.sh
system release

#allbench release structured structured
#bintree release structured structured
#source ./time.sh
#mv times.csv times.ss.csv

#allbench release nonblock structured
#bintree release nonblock structured
#source ./time.sh
#mv times.csv times.ns.csv

#allbench release nonblock nonblock
#bintree release nonblock nonblock
#source ./time.sh
#mv times.csv times.nn.csv
