cd /mnt/future-rd/benchmarks/futurerd-bench/heartWallTracking-baseline/cilk-futures

echo "running hw-gf baseline"
echo -e "on 1 core:\n"
taskset -c 0    ./hw-gf ../data/test.avi 10 1
echo -e "\non 2 cores:\n"
taskset -c 0-1  ./hw-gf ../data/test.avi 10 2
echo -e "\non 4 cores:\n"
taskset -c 0-3  ./hw-gf ../data/test.avi 10 4
echo -e "\non 8 cores:\n"
taskset -c 0-7  ./hw-gf ../data/test.avi 10 8
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./hw-gf ../data/test.avi 10 16
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./hw-gf ../data/test.avi 10 20

cd -
cd /mnt/future-rd/benchmarks/futurerd-bench/heartWallTracking/cilk-futures
echo -e "\nrunning hw-gf reachability"
echo -e "on 1 core:\n"
taskset -c 0    ./hw-gf ../data/test.avi 10 1
echo -e "\non 2 cores:\n"
taskset -c 0-1  ./hw-gf ../data/test.avi 10 2
echo -e "\non 4 cores:\n"
taskset -c 0-3  ./hw-gf ../data/test.avi 10 4
echo -e "\non 8 cores:\n"
taskset -c 0-7  ./hw-gf ../data/test.avi 10 8
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./hw-gf ../data/test.avi 10 16
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./hw-gf ../data/test.avi 10 20

echo -e "\nrunning hw-gf full detection"
echo -e "on 1 core:\n"
taskset -c 0    ./hw-gf-full ../data/test.avi 10 1
echo -e "\non 2 cores:\n"
taskset -c 0-1  ./hw-gf-full ../data/test.avi 10 2
echo -e "\non 4 cores:\n"
taskset -c 0-3  ./hw-gf-full ../data/test.avi 10 4
echo -e "\non 8 cores:\n"
taskset -c 0-7  ./hw-gf-full ../data/test.avi 10 8
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./hw-gf-full ../data/test.avi 10 16
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./hw-gf-full ../data/test.avi 10 20
