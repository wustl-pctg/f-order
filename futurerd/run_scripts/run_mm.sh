cd /mnt/futurerd/bench/mm

echo "running mm baseline"
taskset -c 0    ./matmul-future-base -n 2048

echo -e "\nrunning mm reachability"
taskset -c 0    ./matmul-future-reach -n 2048

echo -e "\nrunning mm full detection"
taskset -c 0    ./matmul-future-rd -n 2048

