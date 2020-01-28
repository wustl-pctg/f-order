cd /mnt/futurerd/bench/sw-gf

echo "running sw-gf baseline"
taskset -c 0    ./sw-base -n 2048

echo -e "\nrunning sw-gf reachability"
taskset -c 0    ./sw-reach -n 2048

echo -e "\nrunning sw-gf full detection"
taskset -c 0    ./sw-rd -n 2048

