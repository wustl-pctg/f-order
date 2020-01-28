#!/bin/bash
# Requires bash 4+
# Requires GNU datamash 1.3+
# You can run this script directly from the shell or source it in
# another script, e.g., run.sh
set -e

default_progs=(lcs sw matmul_z hw dedup)
PROGS=( "${PROGS[@]:-"${default_progs[@]}"}" )
default_btypes=(base reach inst rd)
BTYPES=( "${BTYPES[@]:-"${default_btypes[@]}"}" )

ITER=2
TIMELOG=$(pwd)/times.csv
OUTPUTLOG=$(pwd)/out.log

# chrt doesn't work without sudo...something wrong on this machine I
# can't figure out ("chrt -f 99 ..." would be nice)
TIME="/usr/bin/time -f %E"

BIG_TREE_SIZE=$((8192 * 1024))
declare -A BIG_ARGS=(
    [lcs]="-n $((1024 * 16))"
    [sw]="-n 2048"
    [matmul_z]="-n 2048"
    [hw]="../data/test.avi 10 1"
    [bt]="../data/simlarge/sequenceB_4 4 4 4000 5 4 1 1"
    [dedup]="-c -i ../data/simlarge/media.dat -o ../out"
    [merge]="-s1 $BIG_TREE_SIZE -s2 $(( $BIG_TREE_SIZE / 2 )) -kmax $(( $BIG_TREE_SIZE * 4 ))"
)

# Small sizes for quick testing
SMALL_TREE_SIZE=$((1024 * 64))
declare -A SMALL_ARGS=(
    [lcs]="-n 32"
    [sw]="-n 32"
    [matmul_z]="-n 256"
    [hw]="../data/test.avi 1 1"
    [bt]="../data/simdev/sequenceB_1 4 1 100 3 4 1 1" # See run.sh in bodytrack
    [dedup]="-c -i ../data/simsmall/media.dat -o ../out"
    [merge]="-s1 $SMALL_TREE_SIZE -s2 $(( $SMALL_TREE_SIZE / 2 )) -kmax $(( $SMALL_TREE_SIZE * 4 ))" 
)

declare -n ARGS=BIG_ARGS

declare -A DIRS
DIRS=([lcs]="basic/" [sw]="basic/" [matmul_z]="basic/"
      [hw]="heartWallTracking/cilk-futures/"
      [bt]="pipeline/bodytrack/build/"
      [dedup]="pipeline/dedup/build/"
      [merge]="bintree/"
      )

# Log setup
SEP=","
HEADERS=(Benchmark Args Type Mean Sstdev)
rm -f $TIMELOG $OUTPUTLOG
for h in ${HEADERS[@]}; do
    printf "${h}${SEP}" >> $TIMELOG
done
printf "\n" >> $TIMELOG

# Run benchmarks
for bench in ${PROGS[@]}; do
    for btype in ${BTYPES[@]}; do
        printf "Running $bench-$btype ${ARGS[$bench]} $ITER times\n"
        pushd ${DIRS[$bench]} >/dev/null

        results=""
        for i in $(seq 1 $ITER); do
            run="./${bench}-${btype} 2>&1 ${ARGS[$bench]}"
            echo "$run"
            tmp=$(eval $run)
            echo "$tmp" >> $OUTPUTLOG

            if [[ "$?" -ne 0 ]]; then
                echo "Execution failed:"
                echo "$tmp";
                exit 1;
            fi

            gather="grep 'Benchmark time' | cut -d ':' -f 2 | cut -d ' ' -f 2"
            results+=$(eval "echo \"$tmp\" | $gather")
            results+="\n"
        done
				# datamash -R option requires version 1.3, Ubuntu pkg is 1.2 currently.
        #stats=$(printf "$results" | datamash -R 2 mean 1 sstdev 1)
				stats=$(printf "$results" | datamash mean 1 sstdev 1)
        avg=$(echo "$stats" | cut -f 1)
        stdev=$(echo "$stats" | cut -f 2)
        outstr="${bench}${SEP}${ARGS[$bench]}${SEP}"
        outstr="${outstr}${btype}${SEP}%.2f${SEP}%.2f${SEP}\n"
        printf "$outstr" ${avg} ${stdev} >> $TIMELOG # Use %.2f in outstr
        popd >/dev/null
    done
done

printf -- "-------------------- Results --------------------\n"
cat $TIMELOG | column -s $SEP -t
