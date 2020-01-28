cd /mnt/future-rd/benchmarks/futurerd-bench/heartWallTracking-baseline/cilk-futures

echo "running hw-sf baseline"
echo -e "on 1 core:\n"
taskset -c 0    ./hw-sf ../data/test.avi 10 1
echo -e "\non 2 cores:\n"
taskset -c 0-1  ./hw-sf ../data/test.avi 10 2
echo -e "\non 4 cores:\n"
taskset -c 0-3  ./hw-sf ../data/test.avi 10 4
echo -e "\non 8 cores:\n"
taskset -c 0-7  ./hw-sf ../data/test.avi 10 8
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./hw-sf ../data/test.avi 10 16
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./hw-sf ../data/test.avi 10 20

cd -
cd /mnt/future-rd/benchmarks/futurerd-bench/heartWallTracking/cilk-futures
echo -e "\nrunning hw-sf reachability"
echo -e "on 1 core:\n"
taskset -c 0    ./hw-sf ../data/test.avi 10 1
echo -e "\non 2 cores:\n"
taskset -c 0-1  ./hw-sf ../data/test.avi 10 2
echo -e "\non 4 cores:\n"
taskset -c 0-3  ./hw-sf ../data/test.avi 10 4
echo -e "\non 8 cores:\n"
taskset -c 0-7  ./hw-sf ../data/test.avi 10 8
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./hw-sf ../data/test.avi 10 16
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./hw-sf ../data/test.avi 10 20

echo -e "\nrunning hw-sf full detection"
echo -e "on 1 core:\n"
taskset -c 0    ./hw-sf-full ../data/test.avi 10 1
echo -e "\non 2 cores:\n"
taskset -c 0-1  ./hw-sf-full ../data/test.avi 10 2
echo -e "\non 4 cores:\n"
taskset -c 0-3  ./hw-sf-full ../data/test.avi 10 4
echo -e "\non 8 cores:\n"
taskset -c 0-7  ./hw-sf-full ../data/test.avi 10 8
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./hw-sf-full ../data/test.avi 10 16
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./hw-sf-full ../data/test.avi 10 20
