cd /mnt/futurerd/bench/sw-sf

echo "running sw-sf baseline"
taskset -c 0    ./sw-base -n 2048

echo -e "\nrunning sw-sf reachability"
taskset -c 0    ./sw-reach -n 2048

echo -e "\nrunning sw-sf full detection"
taskset -c 0    ./sw-rd -n 2048

