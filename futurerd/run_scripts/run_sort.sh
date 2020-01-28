cd /mnt/futurerd/bench/sort

echo "running sort baseline"
taskset -c 0    ./cilksort-future-base -n 10000000

echo -e "\nrunning sort reachability"
taskset -c 0    ./cilksort-future-reach -n 10000000

echo -e "\nrunning sort full detection"
taskset -c 0    ./cilksort-future-rd -n 10000000

