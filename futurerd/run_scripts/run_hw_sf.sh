cd /mnt/futurerd/bench/hw-sf/cilk-futures

echo "running hw-sf baseline"
taskset -c 0    ./hw-base ../data/test.avi 10 1

echo -e "\nrunning hw-sf reachability"
taskset -c 0    ./hw-reach ../data/test.avi 10 1

echo -e "\nrunning hw-sf full detection"
taskset -c 0    ./hw-rd ../data/test.avi 10 1

