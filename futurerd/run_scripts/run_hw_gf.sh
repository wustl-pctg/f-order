cd /mnt/futurerd/bench/hw-gf/cilk-futures

echo "running hw-gf baseline"
taskset -c 0    ./hw-base ../data/test.avi 10 1

echo -e "\nrunning hw-gf reachability"
taskset -c 0    ./hw-reach ../data/test.avi 10 1

echo -e "\nrunning hw-gf full detection"
taskset -c 0    ./hw-rd ../data/test.avi 10 1

